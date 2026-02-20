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
#include "roc_core/atomic_int.h"
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
    TestThread(core::nanoseconds_t deadline, core::Semaphore* semaphore)
        : r_(0)
        , deadline_(deadline) {
        semaphore_ = semaphore;
    }

    ~TestThread() {
        join();
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
        (void)semaphore_->timed_wait(deadline_);
        r_ = false;
    }
    core::AtomicInt<int32_t> r_;
    core::nanoseconds_t deadline_;
    core::Semaphore* semaphore_;
};

} // namespace

TEST_GROUP(semaphore) {};

TEST(semaphore, timeout_test) {
    core::Semaphore sem(0);
    roc_log(LogDebug, "ready");
    bool result =
        sem.timed_wait(1 * core::Second + core::timestamp(core::ClockMonotonic));
    if (result) {
        roc_log(LogDebug, "true, unlocked by other threads");
    } else {
        roc_log(LogDebug, "false, timeout");
    }
    CHECK(result == false);
}

TEST(semaphore, block_test) {
    core::Semaphore sem(0);
    roc_log(LogDebug, "ready");
    
    // Use a vector or array of pointers that we'll clean up
    const int num_threads = 10;
    TestThread* threads[num_threads];
    
    for (int i = 0; i < num_threads; i++) {
        threads[i] = new TestThread(
            2 * core::Second + core::timestamp(core::ClockMonotonic), &sem);
        (void)threads[i]->start();
    }
    
    for (int i = 0; i < num_threads; i++) {
        (void)threads[i]->wait_running();
    }
    roc_log(LogDebug, "finish waiting running");
    
    core::sleep_for(core::ClockMonotonic, core::Millisecond * 100);
    
    for (int i = 0; i < num_threads; i++) {
        CHECK(threads[i]->running());
    }
    roc_log(LogDebug, "finish checking running");
    
    for (int i = 0; i < num_threads; i++) {
        sem.post();
    }
    
    core::sleep_for(core::ClockMonotonic, core::Millisecond * 300);
    
    for (int i = 0; i < num_threads; i++) {
        CHECK(!threads[i]->running());
    }
    roc_log(LogDebug, "finish all finished running");
    
    for (int i = 0; i < num_threads; i++) {
        threads[i]->join();
        delete threads[i]; 
    }
}

TEST(semaphore, multiple_post_before_wait) {
    core::Semaphore sem(0);

    // Post multiple times before any waits
    for (int i = 0; i < 5; i++) {
        sem.post();
    }

    // All waits should succeed immediately without blocking
    for (int i = 0; i < 5; i++) {
        bool result = sem.timed_wait(core::timestamp(core::ClockMonotonic));
        CHECK(result == true);
    }

    // Next wait should timeout since counter is back to 0
    bool result =
        sem.timed_wait(100 * core::Millisecond + core::timestamp(core::ClockMonotonic));
    CHECK(result == false);
}

TEST(semaphore, concurrent_post_and_wait) {
    core::Semaphore sem(0);
    const int num_threads = 20;
    TestThread* threads[num_threads];

    // Start threads that will wait
    for (int i = 0; i < num_threads; i++) {
        threads[i] = new TestThread(
            1 * core::Second + core::timestamp(core::ClockMonotonic), &sem);
        (void)threads[i]->start();
    }

    // Wait for all threads to be running and blocked
    core::sleep_for(core::ClockMonotonic, core::Millisecond * 100);
    for (int i = 0; i < num_threads; i++) {
        CHECK(threads[i]->running());
    }

    // Post from multiple iterations to wake them up gradually
    for (int i = 0; i < num_threads; i++) {
        sem.post();
    }

    // All threads should eventually finish
    core::sleep_for(core::ClockMonotonic, core::Millisecond * 200);
    for (int i = 0; i < num_threads; i++) {
        CHECK(!threads[i]->running());
    }

    for (int i = 0; i < num_threads; i++) {
        threads[i]->join();
        delete threads[i]; 
    }
}

} // namespace pipeline
} // namespace roc