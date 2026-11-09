/*
 * Copyright (c) 2026 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/atomic_bool.h"
#include "roc_core/semaphore.h"
#include "roc_core/thread.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

namespace {

enum { NumThreads = 5, NumPosts = 5 };

// Deadline offset for tests that expect the deadline to expire.
const nanoseconds_t ShortTimeout = Millisecond * 10;

// Upper bound for calls that must not block at all.
const nanoseconds_t MaxImmediate = Millisecond * 100;

// Time given to a freshly started thread to actually block inside the semaphore.
const nanoseconds_t SettleDelay = Microsecond * 100;

class BlockingThread : public Thread {
public:
    BlockingThread()
        : sem_(NULL) {
    }

    void init(Semaphore& sem) {
        sem_ = &sem;
    }

    bool running() const {
        return running_;
    }

    void wait_running() {
        while (!running_) {
            sleep_for(ClockMonotonic, Microsecond);
        }
    }

private:
    virtual void run() {
        running_ = true;
        sem_->wait();
        running_ = false;
    }

    Semaphore* sem_;
    AtomicBool running_;
};

class TimedThread : public Thread {
public:
    TimedThread()
        : sem_(NULL)
        , deadline_(0)
        , result_(false) {
    }

    void init(Semaphore& sem, nanoseconds_t deadline) {
        sem_ = &sem;
        deadline_ = deadline;
    }

    // Value returned by timed_wait(); valid only after join().
    bool result() const {
        return result_;
    }

    void wait_running() {
        while (!running_) {
            sleep_for(ClockMonotonic, Microsecond);
        }
    }

private:
    virtual void run() {
        running_ = true;
        result_ = sem_->timed_wait(deadline_);
        running_ = false;
    }

    Semaphore* sem_;
    nanoseconds_t deadline_;
    bool result_;
    AtomicBool running_;
};

} // namespace

TEST_GROUP(semaphore) {};

TEST(semaphore, post_then_wait) {
    Semaphore sem(0);

    const nanoseconds_t start = timestamp(ClockMonotonic);

    sem.post();
    sem.wait();

    CHECK(timestamp(ClockMonotonic) - start < MaxImmediate);
}

TEST(semaphore, initial_counter) {
    Semaphore sem(2);

    const nanoseconds_t start = timestamp(ClockMonotonic);

    sem.wait();
    sem.wait();

    CHECK(timestamp(ClockMonotonic) - start < MaxImmediate);
}

TEST(semaphore, timed_wait_success) {
    Semaphore sem(0);

    sem.post();

    CHECK(sem.timed_wait(timestamp(ClockMonotonic) + ShortTimeout));
}

TEST(semaphore, timed_wait_timeout) {
    Semaphore sem(0);

    const nanoseconds_t start = timestamp(ClockMonotonic);
    CHECK(!sem.timed_wait(start + ShortTimeout));

    const nanoseconds_t elapsed = timestamp(ClockMonotonic) - start;
    CHECK(elapsed >= ShortTimeout / 2);
    CHECK(elapsed < Second);
}

TEST(semaphore, timed_wait_expired_deadline) {
    Semaphore sem(0);

    const nanoseconds_t start = timestamp(ClockMonotonic);
    CHECK(!sem.timed_wait(start - Second));
    CHECK(timestamp(ClockMonotonic) - start < MaxImmediate);
}

TEST(semaphore, timed_wait_ready_expired_deadline) {
    Semaphore sem(0);

    sem.post();

    // Like POSIX sem_timedwait(), a ready semaphore succeeds despite the deadline.
    CHECK(sem.timed_wait(timestamp(ClockMonotonic) - Second));
}

TEST(semaphore, multiple_posts) {
    Semaphore sem(0);

    for (int i = 0; i < NumPosts; i++) {
        sem.post();
    }

    for (int i = 0; i < NumPosts; i++) {
        CHECK(sem.timed_wait(timestamp(ClockMonotonic) + ShortTimeout));
    }

    CHECK(!sem.timed_wait(timestamp(ClockMonotonic) + ShortTimeout));
}

TEST(semaphore, blocking_thread) {
    Semaphore sem(0);

    BlockingThread thr;
    thr.init(sem);
    CHECK(thr.start());

    thr.wait_running();
    sleep_for(ClockMonotonic, SettleDelay);
    CHECK(thr.running());

    sem.post();
    thr.join();
}

TEST(semaphore, multiple_threads) {
    Semaphore sem(0);

    TimedThread threads[NumThreads];

    // Deadline is generous on purpose: it is a safety net, not the thing under test.
    for (size_t i = 0; i < NumThreads; i++) {
        threads[i].init(sem, timestamp(ClockMonotonic) + Second * 5);
        CHECK(threads[i].start());
    }

    for (size_t i = 0; i < NumThreads; i++) {
        threads[i].wait_running();
    }

    for (size_t i = 0; i < NumThreads; i++) {
        sem.post();
    }

    for (size_t i = 0; i < NumThreads; i++) {
        threads[i].join();
        CHECK(threads[i].result());
    }
}

} // namespace core
} // namespace roc
