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
#include "roc_core/thread.h"
#include "roc_core/time.h"
#include "roc_pipeline/state_tracker.h"
#include "roc_sndio/device_defs.h"

namespace roc {
namespace pipeline {

namespace {

enum { NumThreads = 5 };

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

class WaitThread : public core::Thread {
public:
    WaitThread()
        : tracker_(NULL)
        , state_mask_(0)
        , deadline_(0)
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

    void wait_running() {
        while (!running_) {
            core::sleep_for(core::ClockMonotonic, core::Microsecond);
        }
    }

private:
    virtual void run() {
        running_ = true;
        result_ = tracker_->wait_state(state_mask_, deadline_);
        running_ = false;
    }

    StateTracker* tracker_;
    unsigned state_mask_;
    core::nanoseconds_t deadline_;
    bool result_;
    core::AtomicBool running_;
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

} // namespace

TEST_GROUP(state_tracker) {};

TEST(state_tracker, already_active) {
    StateTracker tracker;

    tracker.register_packet();
    CHECK(tracker.get_state() == sndio::DeviceState_Active);

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
    CHECK(tracker.wait_state(sndio::DeviceState_Active, NoDeadline));
    CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);

    tracker.unregister_packet();
}

TEST(state_tracker, already_idle) {
    StateTracker tracker;

    CHECK(tracker.get_state() == sndio::DeviceState_Idle);

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
    CHECK(tracker.wait_state(sndio::DeviceState_Idle, NoDeadline));
    CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);
}

TEST(state_tracker, zero_deadline_match) {
    StateTracker tracker;

    tracker.register_packet();

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
    CHECK(tracker.wait_state(sndio::DeviceState_Active, 0));
    CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);

    tracker.unregister_packet();
}

// Zero deadline behaves like NoDeadline: it blocks until the mask matches.
TEST(state_tracker, zero_deadline_blocks_until_match) {
    StateTracker tracker;

    WaitThread thr;
    thr.init(tracker, sndio::DeviceState_Active, 0);
    CHECK(thr.start());

    thr.wait_running();
    core::sleep_for(core::ClockMonotonic, SettleDelay);

    tracker.register_packet();

    thr.join();
    CHECK(thr.result());

    tracker.unregister_packet();
}

TEST(state_tracker, past_deadline) {
    StateTracker tracker;

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
    CHECK(!tracker.wait_state(sndio::DeviceState_Active, start - core::Second));
    CHECK(core::timestamp(core::ClockMonotonic) - start < MaxImmediate);
}

TEST(state_tracker, empty_mask) {
    StateTracker tracker;

    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);
    CHECK(tracker.wait_state(0, NoDeadline));
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
            CHECK(tracker.wait_state(sndio::DeviceState_Active, NoDeadline));

            signaler.join();
            tracker.unregister_packet();
        }
    }
}

} // namespace pipeline
} // namespace roc
