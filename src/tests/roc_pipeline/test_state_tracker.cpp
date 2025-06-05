/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_harness.h"

#include "roc_address/protocol.h"
#include "roc_audio/mixer.h"
#include "roc_audio/sample.h"
#include "roc_core/atomic.h"
#include "roc_core/heap_arena.h"
#include "roc_core/noop_arena.h"
#include "roc_core/semaphore.h"
#include "roc_core/thread.h"
#include "roc_core/time.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/receiver_endpoint.h"
#include "roc_pipeline/receiver_session_group.h"

namespace roc {
namespace pipeline {

namespace {

enum { PacketSz = 512 };

core::HeapArena arena;

packet::PacketFactory packet_factory(arena, PacketSz);
audio::FrameFactory frame_factory(arena, PacketSz * sizeof(audio::sample_t));

audio::ProcessorMap processor_map(arena);
rtp::EncodingMap encoding_map(arena);

class TestThread : public core::Thread {
public:
    TestThread(StateTracker& st, unsigned int state_mask, core::nanoseconds_t deadline)
        : t_(st)
        , r_(0)
        , state_mask_(state_mask)
        , deadline_(deadline) {
    }

    bool running() const {
        return r_;
    }

    void wait_running() {
        while (!r_) {
            core::sleep_for(core::ClockMonotonic, core::Microsecond);
        }
    }

private:
    virtual void run() {
        r_ = true;
        t_.wait_state(state_mask_, deadline_);
        r_ = false;
    }

    StateTracker& t_;
    core::Atomic<int> r_;
    unsigned int state_mask_;
    core::nanoseconds_t deadline_;
};

} // namespace

TEST_GROUP(state_tracker) {};

// set a thread that last for 0.5 seconds, wait for 1 second to make it timeout.
TEST(state_tracker, simple_timeout) {
    StateTracker state_tracker;
    TestThread thr(state_tracker, sndio::DeviceState_Active,
                   core::timestamp(core::ClockMonotonic) + core::Millisecond * 500);

    CHECK(thr.start());
    core::sleep_for(core::ClockMonotonic, core::Millisecond * 1000);
    CHECK(!(thr.running()));
    thr.join();
}

TEST(state_tracker, multiple_timeout) {
    StateTracker state_tracker;
    TestThread** threads_ptr = new TestThread*[10];

    // set threads that last for 1 second
    for (int i = 0; i < 10; i++) {
        threads_ptr[i] = new TestThread(state_tracker, sndio::DeviceState_Active,
                                        core::timestamp(core::ClockMonotonic)
                                            + core::Millisecond * 1000);
    }

    // wait for start, then check if threads are running
    for (int i = 0; i < 10; i++) {
        CHECK(threads_ptr[i]->start());
        // CHECK(threads_ptr[i]->running());
        // roc_log(LogDebug, "check running %d\n", i);
    }
    core::sleep_for(core::ClockMonotonic, core::Millisecond * 10);
    for (int i = 0; i < 10; i++) {
        // roc_log(LogDebug, "check running %d\n", i);
        CHECK(threads_ptr[i]->running());
    }

    // sleep for 2 seconds, making the threads timeout
    roc_log(LogDebug, "started running");
    core::sleep_for(core::ClockMonotonic, core::Millisecond * 2000);

    // check if threads are stopped
    for (int i = 0; i < 10; i++) {
        CHECK(!threads_ptr[i]->running());
    }

    roc_log(LogDebug, "started joining");

    for (int i = 0; i < 10; i++) {
        threads_ptr[i]->join();
    }

    roc_log(LogDebug, "finished joining");

    for (int i = 0; i < 10; ++i) {
        delete threads_ptr[i];
    }
    delete[] threads_ptr;
}

TEST(state_tracker, multiple_switch) {
    StateTracker state_tracker;
    TestThread** threads_ptr = new TestThread*[10];

    // set threads without waiting time
    for (int i = 0; i < 10; i++) {
        threads_ptr[i] = new TestThread(state_tracker, sndio::DeviceState_Active, -1);
    }

    for (int i = 0; i < 10; i++) {
        CHECK(threads_ptr[i]->start());
    }

    roc_log(LogDebug, "started running");

    // wait for threads starting
    core::sleep_for(core::ClockMonotonic, core::Millisecond * 500);

    // check if the threads have started
    for (int i = 0; i < 10; i++) {
        CHECK(threads_ptr[i]->running());
    }

    // register a packet
    core::sleep_for(core::ClockMonotonic, core::Millisecond * 500);
    state_tracker.register_packet();
    core::sleep_for(core::ClockMonotonic, core::Millisecond * 500);

    // check if the threads have been stopped
    for (int i = 0; i < 10; i++) {
        CHECK(!(threads_ptr[i]->running()));
    }

    roc_log(LogDebug, "started joining");
    for (int i = 0; i < 10; i++) {
        threads_ptr[i]->join();
    }
    roc_log(LogDebug, "finished joining");

    for (int i = 0; i < 10; ++i) {
        delete threads_ptr[i];
    }
    delete[] threads_ptr;
}

TEST(state_tracker, semaphore_test) {
    core::Semaphore sem(0);
    roc_log(LogDebug, "ready");
    if (sem.timed_wait(1 * core::Second + core::timestamp(core::ClockMonotonic)))
        roc_log(LogDebug, "true, unlocked by other threads");
    else
        roc_log(LogDebug, "false, timeout");
}
} // namespace pipeline
} // namespace roc
