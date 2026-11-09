/*
 * Copyright (c) 2026 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_harness.h"

#include "roc_core/atomic_bool.h"
#include "roc_core/cpu_instructions.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/thread.h"
#include "roc_core/time.h"
#include "roc_pipeline/state_tracker.h"
#include "roc_sndio/device_defs.h"

namespace roc {
namespace pipeline {

namespace {

enum { NumThreads = 5, HandoverRounds = 5000 };

// Deadline value that means "block forever".
const core::nanoseconds_t NoDeadline = -1;

// Deadline offset for tests that expect the deadline to expire.
const core::nanoseconds_t ShortTimeout = core::Millisecond * 10;

// Upper bound for calls that must not block at all.
const core::nanoseconds_t MaxImmediate = core::Millisecond * 100;

// Time given to a freshly started thread to actually block inside wait_state().
const core::nanoseconds_t SettleDelay = core::Microsecond * 100;

// Deadline long enough to never expire while the test runs.
const core::nanoseconds_t LongTimeout = core::Minute * 10;

// Deadline that a passing test never reaches, but that turns a missed wakeup into a
// failure instead of an infinite hang.
const core::nanoseconds_t ReleaseTimeout = core::Second * 5;

class WaitThread : public core::Thread {
public:
    WaitThread()
        : tracker_(NULL)
        , state_mask_(0)
        , deadline_(0)
        , ret_states_(0)
        , result_(false) {
    }

    void init(StateTracker& tracker, unsigned state_mask, core::nanoseconds_t deadline) {
        tracker_ = &tracker;
        state_mask_ = state_mask;
        deadline_ = deadline;
    }

    // Value returned by wait_state(); valid only after join().
    bool result() const {
        return result_;
    }

    // States reported by wait_state(); valid only after join().
    unsigned ret_states() const {
        return ret_states_;
    }

    void wait_running() {
        while (!running_) {
            core::sleep_for(core::ClockMonotonic, core::Microsecond);
        }
    }

    // True if wait_state() already returned.
    bool is_done() const {
        return done_;
    }

    // Wait until wait_state() returns; false if it didn't return within the timeout.
    bool wait_done(core::nanoseconds_t timeout) {
        const core::nanoseconds_t deadline =
            core::timestamp(core::ClockMonotonic) + timeout;
        while (!done_) {
            if (core::timestamp(core::ClockMonotonic) >= deadline) {
                return false;
            }
            core::sleep_for(core::ClockMonotonic, core::Microsecond);
        }
        return true;
    }

private:
    virtual void run() {
        running_ = true;
        result_ = tracker_->wait_state(state_mask_, ret_states_, deadline_);
        running_ = false;
        done_ = true;
    }

    StateTracker* tracker_;
    unsigned state_mask_;
    core::nanoseconds_t deadline_;
    unsigned ret_states_;
    bool result_;
    core::AtomicBool running_;
    core::AtomicBool done_;
};

class SignalThread : public core::Thread {
public:
    SignalThread()
        : tracker_(NULL)
        , delay_(0) {
    }

    void init(StateTracker& tracker, unsigned delay) {
        tracker_ = &tracker;
        delay_ = delay;
    }

    void unblock() {
        go_ = true;
    }

    void wait_spinning() {
        while (!spinning_) {
            core::cpu_relax();
        }
    }

private:
    virtual void run() {
        spinning_ = true;
        while (!go_) {
            core::cpu_relax();
        }
        for (unsigned n = 0; n < delay_; n++) {
            core::cpu_relax();
        }
        tracker_->register_packet();
    }

    StateTracker* tracker_;
    unsigned delay_;
    core::AtomicBool spinning_;
    core::AtomicBool go_;
};

// Hands over between a pending packet and a session, back and forth, so that the
// tracker stays active all the time. Expects one pending packet at the start.
class HandoverThread : public core::Thread {
public:
    HandoverThread()
        : tracker_(NULL)
        , num_rounds_(0) {
    }

    void init(StateTracker& tracker, size_t num_rounds) {
        tracker_ = &tracker;
        num_rounds_ = num_rounds;
    }

private:
    virtual void run() {
        for (size_t n = 0; n < num_rounds_; n++) {
            tracker_->register_session();
            tracker_->unregister_packet();
            tracker_->register_packet();
            tracker_->unregister_session();
        }
    }

    StateTracker* tracker_;
    size_t num_rounds_;
};

// Reads tracker state in a loop until stopped, and counts how often it looked idle.
class IdleObserverThread : public core::Thread {
public:
    IdleObserverThread()
        : tracker_(NULL)
        , num_reads_(0)
        , num_idle_(0) {
    }

    void init(StateTracker& tracker) {
        tracker_ = &tracker;
    }

    void wait_running() {
        while (!running_) {
            core::sleep_for(core::ClockMonotonic, core::Microsecond);
        }
    }

    void stop() {
        stop_ = true;
    }

    // Valid only after join().
    size_t num_reads() const {
        return num_reads_;
    }

    // Valid only after join().
    size_t num_idle() const {
        return num_idle_;
    }

private:
    virtual void run() {
        running_ = true;
        do {
            if (tracker_->get_state() == sndio::DeviceState_Idle) {
                num_idle_++;
            }
            unsigned ret_states = 0;
            if (tracker_->wait_state(sndio::DeviceState_Idle, ret_states, 0)) {
                num_idle_++;
            }
            num_reads_++;
        } while (!stop_);
    }

    StateTracker* tracker_;
    size_t num_reads_;
    size_t num_idle_;
    core::AtomicBool running_;
    core::AtomicBool stop_;
};

} // namespace

TEST_GROUP(state_tracker) {};

TEST(state_tracker, already_active) {
    StateTracker tracker;

    tracker.register_packet();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);

    unsigned ret_states = 0;
    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
    CHECK(tracker.wait_state(sndio::DeviceState_Active, ret_states, NoDeadline));
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Active, ret_states);
    CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);

    tracker.unregister_packet();
}

TEST(state_tracker, already_idle) {
    StateTracker tracker;

    CHECK(tracker.get_state() == sndio::DeviceState_Idle);

    unsigned ret_states = 0;
    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
    CHECK(tracker.wait_state(sndio::DeviceState_Idle, ret_states, NoDeadline));
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Idle, ret_states);
    CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);
}

TEST(state_tracker, zero_deadline_match) {
    StateTracker tracker;

    unsigned ret_states = 0;
    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    CHECK(tracker.wait_state(sndio::DeviceState_Idle, ret_states, 0));
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Idle, ret_states);

    tracker.register_packet();

    CHECK(tracker.wait_state(sndio::DeviceState_Active, ret_states, 0));
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Active, ret_states);

    tracker.unregister_packet();

    CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);
}

// Halt state is matched by zero deadline even if it's not in the mask.
TEST(state_tracker, zero_deadline_halt_state) {
    StateTracker tracker;

    tracker.set_closed();

    unsigned ret_states = 0;
    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
    CHECK(tracker.wait_state(sndio::DeviceState_Active, ret_states, 0));
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Closed, ret_states);
    CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);
}

// Zero deadline checks the state once and returns immediately, even if nothing matches.
TEST(state_tracker, zero_deadline_no_match) {
    StateTracker tracker;

    unsigned ret_states = 123;
    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    CHECK(!tracker.wait_state(sndio::DeviceState_Active, ret_states, 0));
    UNSIGNED_LONGS_EQUAL(0, ret_states);

    tracker.register_packet();

    ret_states = 123;
    CHECK(!tracker.wait_state(sndio::DeviceState_Idle, ret_states, 0));
    UNSIGNED_LONGS_EQUAL(0, ret_states);

    CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);

    tracker.unregister_packet();
}

// Negative deadline, no matter how negative, means no deadline, unlike zero.
TEST(state_tracker, negative_deadline_blocks_until_match) {
    const core::nanoseconds_t deadlines[] = { -1, -core::Second,
                                              ROC_MIN_OF(core::nanoseconds_t) };

    for (size_t i = 0; i < ROC_ARRAY_SIZE(deadlines); i++) {
        StateTracker tracker;

        // One of the threads becomes semaphore owner, and another one sleeps on condvar.
        WaitThread threads[2];
        for (size_t j = 0; j < 2; j++) {
            threads[j].init(tracker, sndio::DeviceState_Active, deadlines[i]);
            CHECK(threads[j].start());
        }

        for (size_t j = 0; j < 2; j++) {
            threads[j].wait_running();
        }

        core::sleep_for(core::ClockMonotonic, ShortTimeout);

        for (size_t j = 0; j < 2; j++) {
            CHECK(!threads[j].is_done());
        }

        tracker.register_packet();

        for (size_t j = 0; j < 2; j++) {
            CHECK(threads[j].wait_done(ReleaseTimeout));
            threads[j].join();
            CHECK(threads[j].result());
            UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Active, threads[j].ret_states());
        }

        tracker.unregister_packet();
    }
}

// Deadline in the past (but not negative) is expired, it doesn't mean no deadline.
TEST(state_tracker, past_deadline) {
    StateTracker tracker;

    unsigned ret_states = 123;
    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
    CHECK(!tracker.wait_state(sndio::DeviceState_Active, ret_states, 1));
    UNSIGNED_LONGS_EQUAL(0, ret_states);
    CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);
}

// State is checked before the deadline, so past deadline still matches.
TEST(state_tracker, past_deadline_match) {
    StateTracker tracker;

    tracker.register_session();

    unsigned ret_states = 0;
    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
    CHECK(tracker.wait_state(sndio::DeviceState_Active, ret_states, 1));
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Active, ret_states);
    CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);

    tracker.unregister_session();
}

TEST(state_tracker, empty_mask) {
    StateTracker tracker;

    unsigned ret_states = 0;
    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
    CHECK(tracker.wait_state(0, ret_states, NoDeadline));
    UNSIGNED_LONGS_EQUAL(0, ret_states);
    CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);
}

TEST(state_tracker, timeout) {
    StateTracker tracker;

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Active,
             core::timestamp(core::ClockMonotonic) + ShortTimeout);
    CHECK(thr.start());

    // Nothing ever changes state, so the thread can exit only when deadline expires.
    thr.join();
    CHECK(!thr.result());
}

TEST(state_tracker, wakeup_on_register_packet) {
    StateTracker tracker;

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Active, NoDeadline);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.register_packet();

    thr.join();
    CHECK(thr.result());

    tracker.unregister_packet();
}

TEST(state_tracker, wakeup_on_register_session) {
    StateTracker tracker;

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Active, NoDeadline);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.register_session();

    thr.join();
    CHECK(thr.result());

    tracker.unregister_session();
}

TEST(state_tracker, wakeup_on_idle) {
    StateTracker tracker;

    tracker.register_packet();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Idle, NoDeadline);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.unregister_packet();

    thr.join();
    CHECK(thr.result());
}

TEST(state_tracker, wakeup_on_broken) {
    StateTracker tracker;

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Broken, NoDeadline);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.set_broken();

    thr.join();
    CHECK(thr.result());
}

TEST(state_tracker, wakeup_on_closed) {
    StateTracker tracker;

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Closed, NoDeadline);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.set_closed();

    thr.join();
    CHECK(thr.result());
}

// Broken and closed are terminal, so they must release waiters that didn't ask for them.
TEST(state_tracker, wakeup_on_broken_not_in_mask) {
    StateTracker tracker;

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Active, start + ReleaseTimeout);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.set_broken();

    thr.join();
    CHECK(thr.result());
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Broken, thr.ret_states());
    CHECK(core::timestamp(core::ClockMonotonic) - start < ReleaseTimeout);
}

TEST(state_tracker, wakeup_on_closed_not_in_mask) {
    StateTracker tracker;

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Active, start + ReleaseTimeout);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.set_closed();

    thr.join();
    CHECK(thr.result());
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Closed, thr.ret_states());
    CHECK(core::timestamp(core::ClockMonotonic) - start < ReleaseTimeout);
}

// Same as above, but without a deadline, so that the waiter can't be rescued by timeout.
TEST(state_tracker, wakeup_on_closed_not_in_mask_no_deadline) {
    StateTracker tracker;

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Active, NoDeadline);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.set_closed();

    thr.join();
    CHECK(thr.result());
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Closed, thr.ret_states());
}

// Waiter with a mask that doesn't match anything must still be released by the halt
// state, regardless of whether it owns the semaphore or sleeps on the condvar.
TEST(state_tracker, halt_releases_all_waiters) {
    StateTracker tracker;

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    WaitThread threads[NumThreads];

    threads[0].init(tracker, sndio::DeviceState_Active, NoDeadline);
    threads[1].init(tracker, sndio::DeviceState_Paused, NoDeadline);
    threads[2].init(tracker, sndio::DeviceState_Active, start + ReleaseTimeout);
    threads[3].init(tracker, sndio::DeviceState_Paused, start + ReleaseTimeout);
    threads[4].init(tracker,
                    (unsigned)sndio::DeviceState_Active
                        | (unsigned)sndio::DeviceState_Paused,
                    NoDeadline);

    for (size_t i = 0; i < NumThreads; i++) {
        CHECK(threads[i].start());
    }

    for (size_t i = 0; i < NumThreads; i++) {
        threads[i].wait_running();
    }

    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.set_broken();

    for (size_t i = 0; i < NumThreads; i++) {
        threads[i].join();
        CHECK(threads[i].result());
        UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Broken, threads[i].ret_states());
    }

    CHECK(core::timestamp(core::ClockMonotonic) - start < ReleaseTimeout);
}

// Waiting on an already halted tracker doesn't block, even if mask doesn't match.
TEST(state_tracker, already_halted_not_in_mask) {
    {
        StateTracker tracker;
        tracker.set_broken();

        unsigned ret_states = 0;
        const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
        CHECK(tracker.wait_state(sndio::DeviceState_Active, ret_states, NoDeadline));
        UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Broken, ret_states);
        CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);
    }
    {
        StateTracker tracker;
        tracker.set_closed();

        unsigned ret_states = 0;
        const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
        CHECK(tracker.wait_state(sndio::DeviceState_Idle, ret_states, NoDeadline));
        UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Closed, ret_states);
        CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);
    }
}

TEST(state_tracker, multi_bit_mask) {
    StateTracker tracker;

    const unsigned state_mask =
        (unsigned)sndio::DeviceState_Active | (unsigned)sndio::DeviceState_Broken;

    WaitThread thr;
    thr.init(tracker, state_mask, NoDeadline);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.set_broken();

    thr.join();
    CHECK(thr.result());
    // Only the state that actually happened is reported, not the whole mask.
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Broken, thr.ret_states());
}

TEST(state_tracker, concurrent_waiters) {
    StateTracker tracker;

    WaitThread threads[NumThreads];

    for (size_t i = 0; i < NumThreads; i++) {
        threads[i].init(tracker, sndio::DeviceState_Active, NoDeadline);
        CHECK(threads[i].start());
    }

    for (size_t i = 0; i < NumThreads; i++) {
        threads[i].wait_running();
    }

    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.register_packet();

    for (size_t i = 0; i < NumThreads; i++) {
        threads[i].join();
        CHECK(threads[i].result());
    }

    tracker.unregister_packet();
}

// Covers the bug where a waiter that lost the semaphore-owner election passed the
// absolute deadline to the condvar as if it were a relative timeout.
TEST(state_tracker, concurrent_waiters_mixed_deadlines) {
    StateTracker tracker;

    WaitThread thr_a;
    thr_a.init(tracker, sndio::DeviceState_Broken, NoDeadline);
    CHECK(thr_a.start());

    thr_a.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay * 2);

    WaitThread thr_b;
    thr_b.init(tracker, sndio::DeviceState_Broken,
               core::timestamp(core::ClockMonotonic) + ShortTimeout);
    CHECK(thr_b.start());

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
    thr_b.join();
    CHECK(!thr_b.result());
    CHECK(core::timestamp(core::ClockMonotonic) - start < core::Second);

    tracker.set_broken();

    thr_a.join();
    CHECK(thr_a.result());
}

TEST(state_tracker, long_non_expiring_deadline) {
    StateTracker tracker;

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Active,
             core::timestamp(core::ClockMonotonic) + LongTimeout);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.register_packet();

    thr.join();
    CHECK(thr.result());

    tracker.unregister_packet();
}

// Signaler changes state concurrently with the waiter. The delay is swept so that the
// state change eventually lands exactly in between the moment when the waiter checks
// the state and the moment when it announces itself to the signaler. If that window is
// not handled, the signaler skips the wake up and the waiter blocks forever.
TEST(state_tracker, race_signal_and_wait) {
    enum { MaxDelay = 256, NumRepeats = 8 };

    for (unsigned delay = 0; delay < MaxDelay; delay++) {
        for (int rep = 0; rep < NumRepeats; rep++) {
            StateTracker tracker;

            SignalThread signaler;
            signaler.init(tracker, delay);
            CHECK(signaler.start());

            signaler.wait_spinning();
            signaler.unblock();

            unsigned ret_states = 0;
            CHECK(tracker.wait_state(sndio::DeviceState_Active, ret_states, NoDeadline));
            UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Active, ret_states);

            signaler.join();
            tracker.unregister_packet();
        }
    }
}

// Deadlines that are too far in the future are treated as infinite and must not
// overflow the time arithmetic of condvar and semaphore.
TEST(state_tracker, far_future_deadline_already_matched) {
    const core::nanoseconds_t deadlines[] = { ROC_MAX_OF(core::nanoseconds_t),
                                              ROC_MAX_OF(core::nanoseconds_t) - 1 };

    for (size_t i = 0; i < ROC_ARRAY_SIZE(deadlines); i++) {
        StateTracker tracker;

        unsigned ret_states = 0;
        const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

        CHECK(tracker.wait_state(sndio::DeviceState_Idle, ret_states, deadlines[i]));
        UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Idle, ret_states);

        tracker.register_packet();

        CHECK(tracker.wait_state(sndio::DeviceState_Active, ret_states, deadlines[i]));
        UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Active, ret_states);

        tracker.unregister_packet();

        CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);
    }
}

// Several waiters, so that some of them go through the semaphore and some through
// the condvar. None of them should panic or expire, and all should be released.
TEST(state_tracker, far_future_deadline_concurrent_waiters) {
    StateTracker tracker;

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    WaitThread threads[NumThreads];

    threads[0].init(tracker, sndio::DeviceState_Active, ROC_MAX_OF(core::nanoseconds_t));
    threads[1].init(tracker, sndio::DeviceState_Active,
                    ROC_MAX_OF(core::nanoseconds_t) - 1);
    threads[2].init(tracker, sndio::DeviceState_Active, ROC_MAX_OF(core::nanoseconds_t));
    threads[3].init(tracker, sndio::DeviceState_Active, start + core::Day * 365 * 20);
    threads[4].init(tracker, sndio::DeviceState_Active,
                    ROC_MAX_OF(core::nanoseconds_t) - 1);

    for (size_t i = 0; i < NumThreads; i++) {
        CHECK(threads[i].start());
    }

    for (size_t i = 0; i < NumThreads; i++) {
        threads[i].wait_running();
    }

    core::sleep_for(core::ClockMonotonic, ShortTimeout);

    for (size_t i = 0; i < NumThreads; i++) {
        CHECK(!threads[i].is_done());
    }

    tracker.register_packet();

    for (size_t i = 0; i < NumThreads; i++) {
        CHECK(threads[i].wait_done(ReleaseTimeout));
        threads[i].join();
        CHECK(threads[i].result());
        UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Active, threads[i].ret_states());
    }

    tracker.unregister_packet();
}

// Semaphore owner leaves by deadline while another waiter remains. Ownership must be
// handed over, so that a later state change still wakes the remaining waiter.
TEST(state_tracker, handoff_owner_leaves_by_deadline) {
    StateTracker tracker;

    tracker.register_packet();

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    // Starts first, hence is expected to become semaphore owner.
    WaitThread owner;
    owner.init(tracker, sndio::DeviceState_Idle, start + ShortTimeout * 3);
    CHECK(owner.start());

    owner.wait_running();
    core::sleep_for(core::ClockMonotonic, ShortTimeout / 2);

    WaitThread waiter;
    waiter.init(tracker,
                (unsigned)sndio::DeviceState_Idle | (unsigned)sndio::DeviceState_Paused,
                start + ReleaseTimeout);
    CHECK(waiter.start());

    waiter.wait_running();

    owner.join();
    CHECK(!owner.result());

    // Waiter should take the ownership over, and shouldn't be released by that.
    core::sleep_for(core::ClockMonotonic, ShortTimeout);
    CHECK(!waiter.is_done());

    const core::nanoseconds_t change = core::timestamp(core::ClockMonotonic);
    tracker.unregister_packet();

    waiter.join();
    CHECK(waiter.result());
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Idle, waiter.ret_states());
    CHECK(core::timestamp(core::ClockMonotonic) - change < ReleaseTimeout / 2);
}

// Semaphore owner leaves because its mask matched, while another waiter, whose mask
// can't match, remains. Paused is never produced, so this waiter can only be released
// by a halt state, which still must reach it after the ownership was handed over.
TEST(state_tracker, handoff_owner_leaves_by_match) {
    StateTracker tracker;

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    // Starts first, hence is expected to become semaphore owner.
    WaitThread owner;
    owner.init(tracker, sndio::DeviceState_Active, start + ReleaseTimeout);
    CHECK(owner.start());

    owner.wait_running();
    core::sleep_for(core::ClockMonotonic, ShortTimeout / 2);

    WaitThread waiter;
    waiter.init(tracker, sndio::DeviceState_Paused, start + ReleaseTimeout);
    CHECK(waiter.start());

    waiter.wait_running();
    core::sleep_for(core::ClockMonotonic, ShortTimeout / 2);

    tracker.register_packet();

    owner.join();
    CHECK(owner.result());
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Active, owner.ret_states());

    // Further state changes must not release waiter, but they go through the
    // semaphore that it should have taken over.
    tracker.unregister_packet();
    tracker.register_packet();

    core::sleep_for(core::ClockMonotonic, ShortTimeout);
    CHECK(!waiter.is_done());

    const core::nanoseconds_t change = core::timestamp(core::ClockMonotonic);
    tracker.set_closed();

    waiter.join();
    CHECK(waiter.result());
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Closed, waiter.ret_states());
    CHECK(core::timestamp(core::ClockMonotonic) - change < ReleaseTimeout / 2);

    tracker.unregister_packet();
}

// Ownership is handed over several times in a row, as owners leave one by one.
TEST(state_tracker, handoff_chain) {
    StateTracker tracker;

    tracker.register_packet();

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    WaitThread threads[NumThreads];

    for (size_t i = 0; i < NumThreads - 1; i++) {
        threads[i].init(tracker, sndio::DeviceState_Idle,
                        start + ShortTimeout * 2 * core::nanoseconds_t(i + 1));
    }
    threads[NumThreads - 1].init(
        tracker, (unsigned)sndio::DeviceState_Idle | (unsigned)sndio::DeviceState_Paused,
        start + ReleaseTimeout);

    for (size_t i = 0; i < NumThreads; i++) {
        CHECK(threads[i].start());
    }

    for (size_t i = 0; i < NumThreads - 1; i++) {
        threads[i].join();
        CHECK(!threads[i].result());
        UNSIGNED_LONGS_EQUAL(0, threads[i].ret_states());
    }

    core::sleep_for(core::ClockMonotonic, ShortTimeout);
    CHECK(!threads[NumThreads - 1].is_done());

    const core::nanoseconds_t change = core::timestamp(core::ClockMonotonic);
    tracker.unregister_packet();

    threads[NumThreads - 1].join();
    CHECK(threads[NumThreads - 1].result());
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Idle, threads[NumThreads - 1].ret_states());
    CHECK(core::timestamp(core::ClockMonotonic) - change < ReleaseTimeout / 2);
}

TEST(state_tracker, wakeup_on_unregister_session) {
    StateTracker tracker;

    tracker.register_session();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Idle, start + ReleaseTimeout);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.unregister_session();

    thr.join();
    CHECK(thr.result());
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Idle, thr.ret_states());
    CHECK(core::timestamp(core::ClockMonotonic) - start < ReleaseTimeout);
}

// Idle waiter should be released only when both packets and sessions are gone.
TEST(state_tracker, idle_after_packet_then_session) {
    StateTracker tracker;

    tracker.register_packet();

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Idle, start + ReleaseTimeout);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.register_session();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);

    core::sleep_for(core::ClockMonotonic, ShortTimeout);
    CHECK(!thr.is_done());

    tracker.unregister_packet();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);
    UNSIGNED_LONGS_EQUAL(1, tracker.num_sessions());

    core::sleep_for(core::ClockMonotonic, ShortTimeout);
    CHECK(!thr.is_done());

    tracker.unregister_session();
    CHECK(tracker.get_state() == sndio::DeviceState_Idle);

    thr.join();
    CHECK(thr.result());
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Idle, thr.ret_states());
    CHECK(core::timestamp(core::ClockMonotonic) - start < ReleaseTimeout);
}

TEST(state_tracker, idle_after_session_then_packet) {
    StateTracker tracker;

    tracker.register_session();
    tracker.register_packet();

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Idle, start + ReleaseTimeout);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.unregister_session();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);
    UNSIGNED_LONGS_EQUAL(0, tracker.num_sessions());

    core::sleep_for(core::ClockMonotonic, ShortTimeout);
    CHECK(!thr.is_done());

    tracker.unregister_packet();
    CHECK(tracker.get_state() == sndio::DeviceState_Idle);

    thr.join();
    CHECK(thr.result());
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Idle, thr.ret_states());
    CHECK(core::timestamp(core::ClockMonotonic) - start < ReleaseTimeout);
}

TEST(state_tracker, get_state_transitions) {
    StateTracker tracker;

    CHECK(tracker.get_state() == sndio::DeviceState_Idle);
    UNSIGNED_LONGS_EQUAL(0, tracker.num_sessions());

    tracker.register_packet();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);

    tracker.register_session();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);
    UNSIGNED_LONGS_EQUAL(1, tracker.num_sessions());

    tracker.register_packet();
    tracker.register_session();
    UNSIGNED_LONGS_EQUAL(2, tracker.num_sessions());

    tracker.unregister_packet();
    tracker.unregister_session();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);

    tracker.unregister_packet();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);

    tracker.unregister_session();
    CHECK(tracker.get_state() == sndio::DeviceState_Idle);
    UNSIGNED_LONGS_EQUAL(0, tracker.num_sessions());

    tracker.register_session();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);

    tracker.register_packet();
    tracker.unregister_session();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);

    tracker.unregister_packet();
    CHECK(tracker.get_state() == sndio::DeviceState_Idle);
}

// Halt state takes precedence over counters, which keep working after it.
TEST(state_tracker, halt_state_precedence) {
    StateTracker tracker;

    CHECK(tracker.is_usable());
    CHECK(!tracker.is_closed());

    tracker.register_packet();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);

    tracker.set_broken();
    CHECK(tracker.get_state() == sndio::DeviceState_Broken);
    CHECK(!tracker.is_usable());
    CHECK(!tracker.is_closed());

    tracker.unregister_packet();
    CHECK(tracker.get_state() == sndio::DeviceState_Broken);

    tracker.set_closed();
    CHECK(tracker.get_state() == sndio::DeviceState_Closed);
    CHECK(!tracker.is_usable());
    CHECK(tracker.is_closed());
}

// While a packet is being handed over to a session, there is always either a packet
// or a session, so the state is never idle. Reading the two counters one after another
// would sometimes miss both and report a state that never existed.
TEST(state_tracker, no_phantom_idle) {
    StateTracker tracker;

    tracker.register_packet();

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    WaitThread idle_waiter;
    idle_waiter.init(tracker, sndio::DeviceState_Idle, start + LongTimeout);
    CHECK(idle_waiter.start());

    idle_waiter.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    IdleObserverThread observer;
    observer.init(tracker);
    CHECK(observer.start());

    observer.wait_running();

    HandoverThread handover;
    handover.init(tracker, HandoverRounds);
    CHECK(handover.start());

    handover.join();

    observer.stop();
    observer.join();

    CHECK(observer.num_reads() > 0);
    UNSIGNED_LONGS_EQUAL(0, observer.num_idle());

    CHECK(!idle_waiter.is_done());
    CHECK(tracker.get_state() == sndio::DeviceState_Active);

    tracker.unregister_packet();
    CHECK(tracker.get_state() == sndio::DeviceState_Idle);

    CHECK(idle_waiter.wait_done(ReleaseTimeout));
    idle_waiter.join();
    CHECK(idle_waiter.result());
    UNSIGNED_LONGS_EQUAL(sndio::DeviceState_Idle, idle_waiter.ret_states());
}

} // namespace pipeline
} // namespace roc
