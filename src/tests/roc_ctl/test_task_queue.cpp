/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/cond.h"
#include "roc_core/mutex.h"
#include "roc_core/panic.h"
#include "roc_ctl/task_queue.h"

namespace roc {
namespace ctl {

namespace {

class TestTaskQueue : public TaskQueue {
public:
    class Task : public TaskQueue::Task {
    public:
        Task() {
        }
    };

    TestTaskQueue()
        : block_cond_(mutex_)
        , unblock_cond_(mutex_)
        , allow_counter_(MaxTasks)
        , blocked_(false)
        , n_tasks_(0) {
        memset(tasks_, 0, sizeof(tasks_));
        for (size_t n = 0; n < MaxTasks; n++) {
            results_[n] = -1;
        }
    }

    ~TestTaskQueue() {
        {
            core::Mutex::Lock lock(mutex_);
            allow_counter_ = MaxTasks;
            unblock_cond_.signal();
        }
        stop_and_wait();
    }

    size_t num_tasks() const {
        core::Mutex::Lock lock(mutex_);
        return n_tasks_;
    }

    const Task* nth_task(size_t n) const {
        core::Mutex::Lock lock(mutex_);
        roc_panic_if_not(n < n_tasks_);
        roc_panic_if_not(tasks_[n]);
        return (Task*)tasks_[n];
    }

    void set_nth_result(size_t n, bool success) {
        core::Mutex::Lock lock(mutex_);
        roc_panic_if_not(n < MaxTasks);
        results_[n] = (success ? TaskSucceeded : TaskFailed);
    }

    void block() {
        core::Mutex::Lock lock(mutex_);
        allow_counter_ = 0;
    }

    void unblock_one() {
        core::Mutex::Lock lock(mutex_);
        allow_counter_++;
        unblock_cond_.signal();
    }

    void wait_blocked() {
        core::Mutex::Lock lock(mutex_);
        while (!blocked_) {
            block_cond_.wait();
        }
    }

    void check_all_unblocked() {
        core::Mutex::Lock lock(mutex_);
        UNSIGNED_LONGS_EQUAL(0, allow_counter_);
        CHECK(!blocked_);
    }

private:
    enum { MaxTasks = 100 };

    virtual TaskResult process_task_imp(TaskQueue::Task& task) {
        core::Mutex::Lock lock(mutex_);
        while (allow_counter_ == 0) {
            blocked_ = true;
            block_cond_.signal();
            unblock_cond_.wait();
        }
        allow_counter_--;
        blocked_ = false;
        roc_panic_if_not(n_tasks_ < MaxTasks);
        size_t n = n_tasks_++;
        tasks_[n] = (Task*)&task;
        roc_panic_if(results_[n] == -1);
        return (TaskResult)results_[n];
    }

    core::Mutex mutex_;
    core::Cond block_cond_;
    core::Cond unblock_cond_;
    size_t allow_counter_;
    bool blocked_;

    size_t n_tasks_;
    Task* tasks_[MaxTasks];
    int results_[MaxTasks];
};

class TestHandler : public TaskQueue::ICompletionHandler {
public:
    TestHandler()
        : cond_(mutex_)
        , task_(NULL)
        , expect_success_(false)
        , expect_cancelled_(false)
        , expect_after_(0)
        , expect_n_calls_(0)
        , actual_calls_(0) {
    }

    ~TestHandler() {
        core::Mutex::Lock lock(mutex_);
        if (actual_calls_ != expect_n_calls_) {
            roc_panic("handler: not enough calls: expected %d call(s), got %d call(s)",
                      (int)expect_n_calls_, (int)actual_calls_);
        }
        if (task_) {
            roc_panic("handler: forgot to invoke wait_called()");
        }
    }

    void expect_success(bool success) {
        core::Mutex::Lock lock(mutex_);
        expect_success_ = success;
    }

    void expect_cancelled(bool cancelled) {
        core::Mutex::Lock lock(mutex_);
        expect_cancelled_ = cancelled;
    }

    void expect_after(core::nanoseconds_t delay) {
        core::Mutex::Lock lock(mutex_);
        expect_after_ = core::timestamp() + delay;
    }

    void expect_n_calls(size_t n) {
        core::Mutex::Lock lock(mutex_);
        expect_n_calls_ += n;
    }

    TestTaskQueue::Task* wait_called() {
        core::Mutex::Lock lock(mutex_);
        while (!task_) {
            cond_.wait();
        }
        TestTaskQueue::Task* ret = (TestTaskQueue::Task*)task_;
        task_ = NULL;
        return ret;
    }

    virtual void control_task_finished(TaskQueue::Task& task) {
        core::Mutex::Lock lock(mutex_);
        if (actual_calls_ == expect_n_calls_) {
            roc_panic("handler: unexpected call: expected only %d call(s)",
                      (int)expect_n_calls_);
        }
        actual_calls_++;
        if (task.success() != expect_success_) {
            roc_panic("handler: unexpected task success status: expected=%d actual=%d",
                      (int)expect_success_, (int)task.success());
        }
        if (task.cancelled() != expect_cancelled_) {
            roc_panic(
                "handler: unexpected task cancellation status: expected=%d actual=%d",
                (int)expect_cancelled_, (int)task.cancelled());
        }
        if (core::timestamp() < expect_after_) {
            roc_panic("handler: task was executed too early");
        }
        task_ = &task;
        cond_.broadcast();
    }

private:
    core::Mutex mutex_;
    core::Cond cond_;

    TaskQueue::Task* task_;

    bool expect_success_;
    bool expect_cancelled_;
    core::nanoseconds_t expect_after_;

    size_t expect_n_calls_;
    size_t actual_calls_;
};

core::nanoseconds_t now_plus_delay(core::nanoseconds_t delay) {
    return core::timestamp() + delay;
}

} // namespace

TEST_GROUP(task_queue) {};

TEST(task_queue, noop) {
    TestTaskQueue tq;
    CHECK(tq.valid());
}

TEST(task_queue, schedule_one) {
    { // success
        TestTaskQueue tq;
        CHECK(tq.valid());

        UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

        TestHandler handler;
        handler.expect_success(true);
        handler.expect_cancelled(false);
        handler.expect_n_calls(1);

        TestTaskQueue::Task task;
        tq.set_nth_result(0, true);
        tq.schedule(task, &handler);

        CHECK(handler.wait_called() == &task);

        UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
        CHECK(tq.nth_task(0) == &task);

        CHECK(task.success());
        CHECK(!task.cancelled());
    }
    { // failure
        TestTaskQueue tq;
        CHECK(tq.valid());

        UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

        TestHandler handler;
        handler.expect_success(false);
        handler.expect_cancelled(false);
        handler.expect_n_calls(1);

        TestTaskQueue::Task task;
        tq.set_nth_result(0, false);
        tq.schedule(task, &handler);

        CHECK(handler.wait_called() == &task);

        UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
        CHECK(tq.nth_task(0) == &task);

        CHECK(!task.success());
        CHECK(!task.cancelled());
    }
}

TEST(task_queue, schedule_one_no_handler) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestTaskQueue::Task task;

    CHECK(!task.success());
    CHECK(!task.cancelled());

    tq.set_nth_result(0, true);
    tq.schedule(task, NULL);

    while (task.pending()) {
        core::sleep_for(core::Microsecond * 100);
    }

    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
    CHECK(tq.nth_task(0) == &task);

    CHECK(task.success());
    CHECK(!task.cancelled());
}

TEST(task_queue, schedule_many_sequantial) {
    enum { NumTasks = 20 };

    TestTaskQueue tq;
    CHECK(tq.valid());

    for (size_t n = 0; n < NumTasks; n++) {
        UNSIGNED_LONGS_EQUAL(n, tq.num_tasks());

        const bool success = (n % 3 != 0);

        TestHandler handler;
        handler.expect_success(success);
        handler.expect_cancelled(false);
        handler.expect_n_calls(1);

        TestTaskQueue::Task task;
        tq.set_nth_result(n, success);
        tq.schedule(task, &handler);

        CHECK(handler.wait_called() == &task);

        UNSIGNED_LONGS_EQUAL(n + 1, tq.num_tasks());
        CHECK(tq.nth_task(n) == &task);

        CHECK(task.success() == success);
        CHECK(!task.cancelled());
    }
}

TEST(task_queue, schedule_many_batched) {
    enum { NumTasks = 20 };

    TestTaskQueue tq;
    CHECK(tq.valid());

    TestTaskQueue::Task tasks[NumTasks];
    TestHandler handlers[NumTasks];

    handlers[0].expect_success(false);
    handlers[0].expect_cancelled(false);
    handlers[0].expect_n_calls(1);

    tq.block();

    tq.set_nth_result(0, false);
    tq.schedule(tasks[0], &handlers[0]);

    tq.wait_blocked();

    for (size_t n = 1; n < NumTasks; n++) {
        const bool success = (n % 3 != 0);

        handlers[n].expect_success(success);
        handlers[n].expect_cancelled(false);
        handlers[n].expect_n_calls(1);

        tq.set_nth_result(n, success);
        tq.schedule(tasks[n], &handlers[n]);
    }

    for (size_t n = 0; n < NumTasks; n++) {
        tq.unblock_one();

        const bool success = (n % 3 != 0);

        CHECK(handlers[n].wait_called() == &tasks[n]);

        UNSIGNED_LONGS_EQUAL(n + 1, tq.num_tasks());
        CHECK(tq.nth_task(n) == &tasks[n]);

        CHECK(tasks[n].success() == success);
        CHECK(!tasks[n].cancelled());
    }

    tq.check_all_unblocked();
}

TEST(task_queue, schedule_and_wait_one) {
    { // success
        TestTaskQueue tq;
        CHECK(tq.valid());

        UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

        TestTaskQueue::Task task;
        tq.set_nth_result(0, true);
        tq.schedule(task, NULL);
        tq.wait(task);

        UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
        CHECK(tq.nth_task(0) == &task);

        CHECK(task.success());
        CHECK(!task.cancelled());
    }
    { // failure
        TestTaskQueue tq;
        CHECK(tq.valid());

        UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

        TestTaskQueue::Task task;
        tq.set_nth_result(0, false);
        tq.schedule(task, NULL);
        tq.wait(task);

        UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
        CHECK(tq.nth_task(0) == &task);

        CHECK(!task.success());
        CHECK(!task.cancelled());
    }
}

TEST(task_queue, schedule_and_wait_many) {
    enum { NumTasks = 20 };

    TestTaskQueue tq;
    CHECK(tq.valid());

    for (size_t n = 0; n < NumTasks; n++) {
        UNSIGNED_LONGS_EQUAL(n, tq.num_tasks());

        const bool success = (n % 3 != 0);

        TestTaskQueue::Task task;
        tq.set_nth_result(n, success);
        tq.schedule(task, NULL);
        tq.wait(task);

        UNSIGNED_LONGS_EQUAL(n + 1, tq.num_tasks());
        CHECK(tq.nth_task(n) == &task);

        CHECK(task.success() == success);
        CHECK(!task.cancelled());
    }
}

TEST(task_queue, schedule_at_one) {
    { // success
        TestTaskQueue tq;
        CHECK(tq.valid());

        UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

        TestHandler handler;
        handler.expect_success(true);
        handler.expect_cancelled(false);
        handler.expect_after(core::Millisecond);
        handler.expect_n_calls(1);

        TestTaskQueue::Task task;
        tq.set_nth_result(0, true);
        tq.schedule_at(task, now_plus_delay(core::Millisecond), &handler);

        CHECK(handler.wait_called() == &task);

        UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
        CHECK(tq.nth_task(0) == &task);

        CHECK(task.success());
        CHECK(!task.cancelled());
    }
    { // failure
        TestTaskQueue tq;
        CHECK(tq.valid());

        UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

        TestHandler handler;
        handler.expect_success(false);
        handler.expect_cancelled(false);
        handler.expect_after(core::Millisecond);
        handler.expect_n_calls(1);

        TestTaskQueue::Task task;
        tq.set_nth_result(0, false);
        tq.schedule_at(task, now_plus_delay(core::Millisecond), &handler);

        CHECK(handler.wait_called() == &task);

        UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
        CHECK(tq.nth_task(0) == &task);

        CHECK(!task.success());
        CHECK(!task.cancelled());
    }
}

TEST(task_queue, schedule_at_one_no_handler) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestTaskQueue::Task task;

    CHECK(!task.success());
    CHECK(!task.cancelled());

    tq.set_nth_result(0, true);
    tq.schedule_at(task, now_plus_delay(core::Millisecond), NULL);

    while (task.pending()) {
        core::sleep_for(core::Microsecond * 100);
    }

    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
    CHECK(tq.nth_task(0) == &task);

    CHECK(task.success());
    CHECK(!task.cancelled());
}

TEST(task_queue, schedule_at_many) {
    enum { NumTasks = 20 };

    TestTaskQueue tq;
    CHECK(tq.valid());

    TestTaskQueue::Task tasks[NumTasks];
    TestHandler handlers[NumTasks];

    handlers[0].expect_success(false);
    handlers[0].expect_cancelled(false);
    handlers[0].expect_n_calls(1);

    tq.block();

    tq.set_nth_result(0, false);
    tq.schedule(tasks[0], &handlers[0]);

    tq.wait_blocked();

    for (size_t n = 1; n < NumTasks; n++) {
        core::sleep_for(core::Microsecond);

        const bool success = (n % 3 != 0);

        const core::nanoseconds_t delay =
            core::Millisecond + core::Microsecond * core::nanoseconds_t(n);

        handlers[n].expect_success(success);
        handlers[n].expect_cancelled(false);
        handlers[n].expect_after(delay);
        handlers[n].expect_n_calls(1);

        tq.set_nth_result(n, success);
        tq.schedule_at(tasks[n], now_plus_delay(delay), &handlers[n]);
    }

    for (size_t n = 0; n < NumTasks; n++) {
        tq.unblock_one();

        const bool success = (n % 3 != 0);

        CHECK(handlers[n].wait_called() == &tasks[n]);

        UNSIGNED_LONGS_EQUAL(n + 1, tq.num_tasks());
        CHECK(tq.nth_task(n) == &tasks[n]);

        CHECK(tasks[n].success() == success);
        CHECK(!tasks[n].cancelled());
    }

    tq.check_all_unblocked();
}

TEST(task_queue, schedule_at_reversed) {
    enum { NumTasks = 20 };

    TestTaskQueue tq;
    CHECK(tq.valid());

    TestTaskQueue::Task tasks[NumTasks];
    TestHandler handlers[NumTasks];

    handlers[0].expect_success(false);
    handlers[0].expect_cancelled(false);
    handlers[0].expect_n_calls(1);

    tq.block();

    tq.set_nth_result(0, false);
    tq.schedule(tasks[0], &handlers[0]);

    tq.wait_blocked();

    const core::nanoseconds_t now = core::timestamp();

    for (size_t n = 1; n < NumTasks; n++) {
        const bool success = (n % 3 != 0);

        const core::nanoseconds_t delay =
            core::Millisecond * core::nanoseconds_t(NumTasks - n);

        handlers[n].expect_success(success);
        handlers[n].expect_cancelled(false);
        handlers[n].expect_n_calls(1);

        tq.set_nth_result(NumTasks - n, success);
        tq.schedule_at(tasks[n], now + delay, &handlers[n]);
    }

    tq.unblock_one();

    CHECK(handlers[0].wait_called() == &tasks[0]);
    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());

    for (size_t n = 1; n < NumTasks; n++) {
        tq.unblock_one();

        const size_t idx = (NumTasks - n);

        const bool success = (idx % 3 != 0);

        CHECK(handlers[idx].wait_called() == &tasks[idx]);

        UNSIGNED_LONGS_EQUAL(n + 1, tq.num_tasks());
        CHECK(tq.nth_task(n) == &tasks[idx]);

        CHECK(tasks[idx].success() == success);
        CHECK(!tasks[idx].cancelled());
    }

    tq.check_all_unblocked();
}

TEST(task_queue, schedule_at_shuffled) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestHandler handler;
    handler.expect_success(true);
    handler.expect_n_calls(4);

    TestTaskQueue::Task tasks[4];

    tq.set_nth_result(0, true);
    tq.set_nth_result(1, true);
    tq.set_nth_result(2, true);
    tq.set_nth_result(3, true);

    tq.block();

    const core::nanoseconds_t now = core::timestamp();

    tq.schedule_at(tasks[0], now + core::Millisecond, &handler);
    tq.schedule_at(tasks[1], now + core::Millisecond * 4, &handler);
    tq.schedule_at(tasks[2], now + core::Millisecond * 2, &handler);
    tq.schedule_at(tasks[3], now + core::Millisecond * 5, &handler);

    tq.unblock_one();
    CHECK(handler.wait_called() == &tasks[0]);
    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());

    tq.unblock_one();
    CHECK(handler.wait_called() == &tasks[2]);
    UNSIGNED_LONGS_EQUAL(2, tq.num_tasks());

    tq.unblock_one();
    CHECK(handler.wait_called() == &tasks[1]);
    UNSIGNED_LONGS_EQUAL(3, tq.num_tasks());

    tq.unblock_one();
    CHECK(handler.wait_called() == &tasks[3]);
    UNSIGNED_LONGS_EQUAL(4, tq.num_tasks());

    CHECK(tasks[0].success());
    CHECK(tasks[1].success());
    CHECK(tasks[2].success());
    CHECK(tasks[3].success());

    tq.check_all_unblocked();
}

TEST(task_queue, schedule_at_same_deadline) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestHandler handler;
    handler.expect_success(true);
    handler.expect_n_calls(4);

    TestTaskQueue::Task tasks[4];

    tq.set_nth_result(0, true);
    tq.set_nth_result(1, true);
    tq.set_nth_result(2, true);
    tq.set_nth_result(3, true);

    tq.block();

    const core::nanoseconds_t now = core::timestamp();

    tq.schedule_at(tasks[0], now + core::Millisecond, &handler);
    tq.schedule_at(tasks[1], now + core::Millisecond * 4, &handler);
    tq.schedule_at(tasks[2], now + core::Millisecond * 4, &handler);
    tq.schedule_at(tasks[3], now + core::Millisecond * 2, &handler);

    tq.unblock_one();
    CHECK(handler.wait_called() == &tasks[0]);
    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());

    tq.unblock_one();
    CHECK(handler.wait_called() == &tasks[3]);
    UNSIGNED_LONGS_EQUAL(2, tq.num_tasks());

    tq.unblock_one();
    CHECK(handler.wait_called() == &tasks[1]);
    UNSIGNED_LONGS_EQUAL(3, tq.num_tasks());

    tq.unblock_one();
    CHECK(handler.wait_called() == &tasks[2]);
    UNSIGNED_LONGS_EQUAL(4, tq.num_tasks());

    CHECK(tasks[0].success());
    CHECK(tasks[1].success());
    CHECK(tasks[2].success());
    CHECK(tasks[3].success());

    tq.check_all_unblocked();
}

TEST(task_queue, schedule_at_and_schedule) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestHandler handler;
    handler.expect_success(true);
    handler.expect_n_calls(4);

    TestTaskQueue::Task tasks[4];

    tq.set_nth_result(0, true);
    tq.set_nth_result(1, true);
    tq.set_nth_result(2, true);
    tq.set_nth_result(3, true);

    tq.block();

    const core::nanoseconds_t now = core::timestamp();

    tq.schedule(tasks[0], &handler);
    tq.schedule_at(tasks[1], now + core::Millisecond * 3, &handler);
    tq.schedule(tasks[2], &handler);
    tq.schedule_at(tasks[3], now + core::Millisecond * 1, &handler);

    tq.unblock_one();
    CHECK(handler.wait_called() == &tasks[0]);
    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());

    tq.unblock_one();
    CHECK(handler.wait_called() == &tasks[2]);
    UNSIGNED_LONGS_EQUAL(2, tq.num_tasks());

    tq.unblock_one();
    CHECK(handler.wait_called() == &tasks[3]);
    UNSIGNED_LONGS_EQUAL(3, tq.num_tasks());

    tq.unblock_one();
    CHECK(handler.wait_called() == &tasks[1]);
    UNSIGNED_LONGS_EQUAL(4, tq.num_tasks());

    CHECK(tasks[0].success());
    CHECK(tasks[1].success());
    CHECK(tasks[2].success());
    CHECK(tasks[3].success());

    tq.check_all_unblocked();
}

TEST(task_queue, schedule_and_async_cancel) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestHandler handlers[4];

    TestTaskQueue::Task tasks[4];

    tq.set_nth_result(0, true);
    tq.set_nth_result(1, true);
    tq.set_nth_result(2, true);
    tq.set_nth_result(3, true);

    handlers[0].expect_success(true);
    handlers[0].expect_cancelled(false);
    handlers[0].expect_n_calls(1);

    handlers[1].expect_success(true);
    handlers[1].expect_cancelled(false);
    handlers[1].expect_n_calls(1);

    handlers[2].expect_success(false);
    handlers[2].expect_cancelled(true);
    handlers[2].expect_n_calls(1);

    handlers[3].expect_success(true);
    handlers[3].expect_cancelled(false);
    handlers[3].expect_n_calls(1);

    tq.block();

    tq.schedule(tasks[0], &handlers[0]);
    tq.schedule(tasks[1], &handlers[1]);
    tq.schedule(tasks[2], &handlers[2]);
    tq.schedule(tasks[3], &handlers[3]);

    tq.wait_blocked();

    tq.async_cancel(tasks[0]);
    tq.async_cancel(tasks[2]);

    tq.unblock_one();
    CHECK(handlers[0].wait_called() == &tasks[0]);
    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
    CHECK(tasks[0].success());
    CHECK(!tasks[0].cancelled());

    tq.unblock_one();
    CHECK(handlers[1].wait_called() == &tasks[1]);
    UNSIGNED_LONGS_EQUAL(2, tq.num_tasks());
    CHECK(tasks[1].success());
    CHECK(!tasks[1].cancelled());

    CHECK(handlers[2].wait_called() == &tasks[2]);
    UNSIGNED_LONGS_EQUAL(2, tq.num_tasks());
    CHECK(!tasks[2].success());
    CHECK(tasks[2].cancelled());

    tq.unblock_one();
    CHECK(handlers[3].wait_called() == &tasks[3]);
    UNSIGNED_LONGS_EQUAL(3, tq.num_tasks());
    CHECK(tasks[3].success());
    CHECK(!tasks[3].cancelled());

    tq.check_all_unblocked();
}

TEST(task_queue, schedule_at_and_async_cancel) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestHandler handlers[4];

    TestTaskQueue::Task tasks[4];

    tq.set_nth_result(0, true);
    tq.set_nth_result(1, true);
    tq.set_nth_result(2, true);
    tq.set_nth_result(3, true);

    handlers[0].expect_success(true);
    handlers[0].expect_cancelled(false);
    handlers[0].expect_n_calls(1);

    handlers[1].expect_success(true);
    handlers[1].expect_cancelled(false);
    handlers[1].expect_n_calls(1);

    handlers[2].expect_success(false);
    handlers[2].expect_cancelled(true);
    handlers[2].expect_n_calls(1);

    handlers[3].expect_success(true);
    handlers[3].expect_cancelled(false);
    handlers[3].expect_n_calls(1);

    tq.block();

    const core::nanoseconds_t now = core::timestamp();

    tq.schedule_at(tasks[0], now + core::Millisecond, &handlers[0]);
    tq.schedule_at(tasks[1], now + core::Millisecond * 4, &handlers[1]);
    tq.schedule_at(tasks[2], now + core::Millisecond * 2, &handlers[2]);
    tq.schedule_at(tasks[3], now + core::Millisecond * 5, &handlers[3]);

    tq.wait_blocked();

    tq.async_cancel(tasks[0]);
    tq.async_cancel(tasks[2]);

    tq.unblock_one();
    CHECK(handlers[0].wait_called() == &tasks[0]);
    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
    CHECK(tasks[0].success());
    CHECK(!tasks[0].cancelled());

    CHECK(handlers[2].wait_called() == &tasks[2]);
    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
    CHECK(!tasks[2].success());
    CHECK(tasks[2].cancelled());

    tq.unblock_one();
    CHECK(handlers[1].wait_called() == &tasks[1]);
    UNSIGNED_LONGS_EQUAL(2, tq.num_tasks());
    CHECK(tasks[1].success());
    CHECK(!tasks[1].cancelled());

    tq.unblock_one();
    CHECK(handlers[3].wait_called() == &tasks[3]);
    UNSIGNED_LONGS_EQUAL(3, tq.num_tasks());
    CHECK(tasks[3].success());
    CHECK(!tasks[3].cancelled());

    tq.check_all_unblocked();
}

TEST(task_queue, cancel_and_wait) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestHandler handler;
    handler.expect_success(false);
    handler.expect_cancelled(true);
    handler.expect_n_calls(1);

    TestTaskQueue::Task task;
    tq.set_nth_result(0, true);

    tq.schedule_at(task, now_plus_delay(core::Second * 999), &handler);
    tq.async_cancel(task);
    tq.wait(task);

    CHECK(!task.success());
    CHECK(task.cancelled());

    CHECK(handler.wait_called() == &task);

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());
}

TEST(task_queue, cancel_already_finished) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    tq.set_nth_result(0, true);

    TestHandler handler;
    handler.expect_success(true);
    handler.expect_cancelled(false);
    handler.expect_n_calls(1);

    TestTaskQueue::Task task;

    tq.schedule(task, &handler);
    CHECK(handler.wait_called() == &task);

    tq.async_cancel(task);

    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
    CHECK(tq.nth_task(0) == &task);

    CHECK(task.success());
    CHECK(!task.cancelled());
}

TEST(task_queue, schedule_already_finished) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    tq.set_nth_result(0, true);
    tq.set_nth_result(1, true);

    TestHandler handler;
    handler.expect_success(true);
    handler.expect_cancelled(false);
    handler.expect_n_calls(2);

    TestTaskQueue::Task task;

    tq.schedule(task, &handler);
    CHECK(handler.wait_called() == &task);

    tq.schedule(task, &handler);
    CHECK(handler.wait_called() == &task);

    UNSIGNED_LONGS_EQUAL(2, tq.num_tasks());
    CHECK(tq.nth_task(0) == &task);
    CHECK(tq.nth_task(1) == &task);

    CHECK(task.success());
    CHECK(!task.cancelled());
}

TEST(task_queue, schedule_at_cancel) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    tq.set_nth_result(0, true);
    tq.set_nth_result(1, true);

    TestHandler handler1;
    TestHandler handler2;

    TestTaskQueue::Task task1;
    TestTaskQueue::Task task2;

    tq.block();

    handler1.expect_success(true);
    handler1.expect_cancelled(false);
    handler1.expect_n_calls(1);

    handler2.expect_success(false);
    handler2.expect_cancelled(true);
    handler2.expect_n_calls(1);

    tq.schedule(task1, &handler1);
    tq.schedule(task2, &handler2);
    tq.async_cancel(task2);

    tq.unblock_one();
    CHECK(handler1.wait_called() == &task1);
    CHECK(handler2.wait_called() == &task2);

    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());

    CHECK(!task2.success());
    CHECK(task2.cancelled());

    handler2.expect_success(true);
    handler2.expect_cancelled(false);
    handler2.expect_n_calls(1);

    tq.schedule(task2, &handler2);

    tq.unblock_one();
    CHECK(handler2.wait_called() == &task2);

    UNSIGNED_LONGS_EQUAL(2, tq.num_tasks());

    CHECK(task2.success());
    CHECK(!task2.cancelled());

    tq.check_all_unblocked();
}

TEST(task_queue, reschedule_new) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestTaskQueue::Task task;
    tq.set_nth_result(0, true);
    tq.reschedule_at(task, now_plus_delay(core::Microsecond * 10));

    while (task.pending()) {
        core::sleep_for(core::Microsecond * 100);
    }

    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
    CHECK(tq.nth_task(0) == &task);

    CHECK(task.success());
    CHECK(!task.cancelled());
}

TEST(task_queue, reschedule_pending) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestHandler handler;

    TestTaskQueue::Task task1;
    TestTaskQueue::Task task2;
    TestTaskQueue::Task task3;

    tq.block();

    tq.set_nth_result(0, true);
    tq.set_nth_result(1, true);
    tq.set_nth_result(2, true);

    tq.schedule(task1, &handler);
    tq.schedule(task2, &handler);
    tq.schedule(task3, &handler);

    tq.wait_blocked();

    tq.reschedule_at(task2, now_plus_delay(core::Millisecond));

    handler.expect_success(true);
    handler.expect_cancelled(false);
    handler.expect_n_calls(1);

    tq.unblock_one();

    CHECK(handler.wait_called() == &task1);

    handler.expect_success(true);
    handler.expect_cancelled(false);
    handler.expect_n_calls(1);

    tq.unblock_one();

    CHECK(handler.wait_called() == &task3);

    handler.expect_success(true);
    handler.expect_cancelled(false);
    handler.expect_n_calls(1);

    tq.unblock_one();

    CHECK(handler.wait_called() == &task2);

    UNSIGNED_LONGS_EQUAL(3, tq.num_tasks());

    CHECK(task1.success());
    CHECK(task2.success());
    CHECK(task3.success());

    tq.check_all_unblocked();
}

TEST(task_queue, reschedule_processing) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestHandler handler;

    TestTaskQueue::Task task;

    tq.block();

    tq.set_nth_result(0, true);
    tq.set_nth_result(1, true);

    tq.schedule(task, &handler);

    tq.wait_blocked();

    tq.reschedule_at(task, now_plus_delay(core::Millisecond));

    handler.expect_success(true);
    handler.expect_cancelled(false);
    handler.expect_n_calls(1);

    tq.unblock_one();

    CHECK(handler.wait_called() == &task);

    handler.expect_success(true);
    handler.expect_cancelled(false);
    handler.expect_n_calls(1);

    tq.unblock_one();

    CHECK(handler.wait_called() == &task);

    CHECK(task.success());
    CHECK(!task.cancelled());

    UNSIGNED_LONGS_EQUAL(2, tq.num_tasks());

    tq.check_all_unblocked();
}

TEST(task_queue, reschedule_succeeded) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestHandler handler;

    TestTaskQueue::Task task;

    handler.expect_success(true);
    handler.expect_cancelled(false);
    handler.expect_n_calls(1);

    tq.set_nth_result(0, true);
    tq.schedule(task, &handler);

    CHECK(handler.wait_called() == &task);

    CHECK(task.success());
    CHECK(!task.cancelled());

    handler.expect_success(true);
    handler.expect_cancelled(false);
    handler.expect_after(core::Millisecond);
    handler.expect_n_calls(1);

    tq.set_nth_result(1, true);
    tq.reschedule_at(task, now_plus_delay(core::Millisecond));

    CHECK(handler.wait_called() == &task);

    CHECK(task.success());
    CHECK(!task.cancelled());

    UNSIGNED_LONGS_EQUAL(2, tq.num_tasks());
}

TEST(task_queue, reschedule_failed) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestHandler handler;

    TestTaskQueue::Task task;

    handler.expect_success(false);
    handler.expect_cancelled(false);
    handler.expect_n_calls(1);

    tq.set_nth_result(0, false);
    tq.schedule(task, &handler);

    CHECK(handler.wait_called() == &task);

    CHECK(!task.success());
    CHECK(!task.cancelled());

    handler.expect_success(true);
    handler.expect_cancelled(false);
    handler.expect_after(core::Millisecond);
    handler.expect_n_calls(1);

    tq.set_nth_result(1, true);
    tq.reschedule_at(task, now_plus_delay(core::Millisecond));

    CHECK(handler.wait_called() == &task);

    CHECK(task.success());
    CHECK(!task.cancelled());

    UNSIGNED_LONGS_EQUAL(2, tq.num_tasks());
}

TEST(task_queue, reschedule_cancelled) {
    TestTaskQueue tq;
    CHECK(tq.valid());

    UNSIGNED_LONGS_EQUAL(0, tq.num_tasks());

    TestHandler handler;

    TestTaskQueue::Task task;

    handler.expect_success(false);
    handler.expect_cancelled(true);
    handler.expect_n_calls(1);

    tq.set_nth_result(0, true);
    tq.schedule_at(task, now_plus_delay(core::Second * 999), &handler);

    tq.async_cancel(task);
    tq.wait(task);

    CHECK(handler.wait_called() == &task);

    CHECK(!task.success());
    CHECK(task.cancelled());

    handler.expect_success(true);
    handler.expect_cancelled(false);
    handler.expect_after(core::Millisecond);
    handler.expect_n_calls(1);

    tq.set_nth_result(1, true);
    tq.reschedule_at(task, now_plus_delay(core::Millisecond));

    CHECK(handler.wait_called() == &task);

    roc_panic_if(!task.success());
    CHECK(task.success());
    CHECK(!task.cancelled());

    UNSIGNED_LONGS_EQUAL(1, tq.num_tasks());
}

} // namespace ctl
} // namespace roc
