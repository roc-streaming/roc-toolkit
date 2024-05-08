/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/cond.h"
#include "roc_core/mutex.h"
#include "roc_core/panic.h"
#include "roc_ctl/control_task_executor.h"
#include "roc_ctl/control_task_queue.h"

namespace roc {
namespace ctl {

namespace {

class TestExecutor : public ControlTaskExecutor<TestExecutor> {
public:
    class Task : public ControlTask {
    public:
        Task()
            : ControlTask(&TestExecutor::do_task_) {
        }
    };

    TestExecutor()
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

    ~TestExecutor() {
        {
            core::Mutex::Lock lock(mutex_);
            allow_counter_ = MaxTasks;
            unblock_cond_.signal();
        }
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
        results_[n] = (success ? ControlTaskSuccess : ControlTaskFailure);
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

    ControlTaskResult do_task_(ControlTask& task) {
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
        return (ControlTaskResult)results_[n];
    }

    core::Mutex mutex_;
    core::Cond block_cond_;
    core::Cond unblock_cond_;
    size_t allow_counter_;
    bool blocked_;

    size_t n_tasks_;
    ControlTask* tasks_[MaxTasks];
    int results_[MaxTasks];
};

class TestCompleter : public IControlTaskCompleter {
public:
    TestCompleter()
        : cond_(mutex_)
        , task_(NULL)
        , expect_success_(false)
        , expect_cancelled_(false)
        , expect_after_(0)
        , expect_n_calls_(0)
        , actual_calls_(0) {
    }

    ~TestCompleter() {
        core::Mutex::Lock lock(mutex_);
        if (actual_calls_ != expect_n_calls_) {
            roc_panic("completer: not enough calls: expected %d call(s), got %d call(s)",
                      (int)expect_n_calls_, (int)actual_calls_);
        }
        if (task_) {
            roc_panic("completer: forgot to invoke wait_called()");
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
        expect_after_ = core::timestamp(core::ClockMonotonic) + delay;
    }

    void expect_n_calls(size_t n) {
        core::Mutex::Lock lock(mutex_);
        expect_n_calls_ += n;
    }

    TestExecutor::Task* wait_called() {
        core::Mutex::Lock lock(mutex_);
        while (!task_) {
            cond_.wait();
        }
        TestExecutor::Task* ret = (TestExecutor::Task*)task_;
        task_ = NULL;
        return ret;
    }

    virtual void control_task_completed(ControlTask& task) {
        core::Mutex::Lock lock(mutex_);
        if (actual_calls_ == expect_n_calls_) {
            roc_panic("completer: unexpected call: expected only %d call(s)",
                      (int)expect_n_calls_);
        }
        actual_calls_++;
        if (task.succeeded() != expect_success_) {
            roc_panic("completer: unexpected task success status: expected=%d actual=%d",
                      (int)expect_success_, (int)task.succeeded());
        }
        if (task.cancelled() != expect_cancelled_) {
            roc_panic(
                "completer: unexpected task cancellation status: expected=%d actual=%d",
                (int)expect_cancelled_, (int)task.cancelled());
        }
        if (core::timestamp(core::ClockMonotonic) < expect_after_) {
            roc_panic("completer: task was executed too early");
        }
        task_ = &task;
        cond_.broadcast();
    }

private:
    core::Mutex mutex_;
    core::Cond cond_;

    ControlTask* task_;

    bool expect_success_;
    bool expect_cancelled_;
    core::nanoseconds_t expect_after_;

    size_t expect_n_calls_;
    size_t actual_calls_;
};

core::nanoseconds_t now_plus_delay(core::nanoseconds_t delay) {
    return core::timestamp(core::ClockMonotonic) + delay;
}

} // namespace

TEST_GROUP(task_queue) {};

TEST(task_queue, noop) {
    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());
}

TEST(task_queue, schedule_one) {
    { // success
        TestExecutor executor;

        ControlTaskQueue queue;
        LONGS_EQUAL(status::StatusOK, queue.init_status());

        UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

        TestCompleter completer;
        completer.expect_success(true);
        completer.expect_cancelled(false);
        completer.expect_n_calls(1);

        TestExecutor::Task task;
        executor.set_nth_result(0, true);
        queue.schedule(task, executor, &completer);

        CHECK(completer.wait_called() == &task);

        UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
        CHECK(executor.nth_task(0) == &task);

        CHECK(task.succeeded());
        CHECK(!task.cancelled());
    }
    { // failure
        TestExecutor executor;

        ControlTaskQueue queue;
        LONGS_EQUAL(status::StatusOK, queue.init_status());

        UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

        TestCompleter completer;
        completer.expect_success(false);
        completer.expect_cancelled(false);
        completer.expect_n_calls(1);

        TestExecutor::Task task;
        executor.set_nth_result(0, false);
        queue.schedule(task, executor, &completer);

        CHECK(completer.wait_called() == &task);

        UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
        CHECK(executor.nth_task(0) == &task);

        CHECK(!task.succeeded());
        CHECK(!task.cancelled());
    }
}

TEST(task_queue, schedule_one_no_completer) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestExecutor::Task task;

    CHECK(!task.succeeded());
    CHECK(!task.cancelled());

    executor.set_nth_result(0, true);
    queue.schedule(task, executor, NULL);

    while (!task.completed()) {
        core::sleep_for(core::ClockMonotonic, core::Microsecond * 100);
    }

    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
    CHECK(executor.nth_task(0) == &task);

    CHECK(task.succeeded());
    CHECK(!task.cancelled());
}

TEST(task_queue, schedule_many_sequantial) {
    enum { NumTasks = 20 };

    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    for (size_t n = 0; n < NumTasks; n++) {
        UNSIGNED_LONGS_EQUAL(n, executor.num_tasks());

        const bool success = (n % 3 != 0);

        TestCompleter completer;
        completer.expect_success(success);
        completer.expect_cancelled(false);
        completer.expect_n_calls(1);

        TestExecutor::Task task;
        executor.set_nth_result(n, success);
        queue.schedule(task, executor, &completer);

        CHECK(completer.wait_called() == &task);

        UNSIGNED_LONGS_EQUAL(n + 1, executor.num_tasks());
        CHECK(executor.nth_task(n) == &task);

        CHECK(task.succeeded() == success);
        CHECK(!task.cancelled());
    }
}

TEST(task_queue, schedule_many_batched) {
    enum { NumTasks = 20 };

    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    TestExecutor::Task tasks[NumTasks];
    TestCompleter completers[NumTasks];

    completers[0].expect_success(false);
    completers[0].expect_cancelled(false);
    completers[0].expect_n_calls(1);

    executor.block();

    executor.set_nth_result(0, false);
    queue.schedule(tasks[0], executor, &completers[0]);

    executor.wait_blocked();

    for (size_t n = 1; n < NumTasks; n++) {
        const bool success = (n % 3 != 0);

        completers[n].expect_success(success);
        completers[n].expect_cancelled(false);
        completers[n].expect_n_calls(1);

        executor.set_nth_result(n, success);
        queue.schedule(tasks[n], executor, &completers[n]);
    }

    for (size_t n = 0; n < NumTasks; n++) {
        executor.unblock_one();

        const bool success = (n % 3 != 0);

        CHECK(completers[n].wait_called() == &tasks[n]);

        UNSIGNED_LONGS_EQUAL(n + 1, executor.num_tasks());
        CHECK(executor.nth_task(n) == &tasks[n]);

        CHECK(tasks[n].succeeded() == success);
        CHECK(!tasks[n].cancelled());
    }

    executor.check_all_unblocked();
}

TEST(task_queue, schedule_and_wait_one) {
    { // success
        TestExecutor executor;

        ControlTaskQueue queue;
        LONGS_EQUAL(status::StatusOK, queue.init_status());

        UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

        TestExecutor::Task task;
        executor.set_nth_result(0, true);
        queue.schedule(task, executor, NULL);
        queue.wait(task);

        UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
        CHECK(executor.nth_task(0) == &task);

        CHECK(task.succeeded());
        CHECK(!task.cancelled());
    }
    { // failure
        TestExecutor executor;

        ControlTaskQueue queue;
        LONGS_EQUAL(status::StatusOK, queue.init_status());

        UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

        TestExecutor::Task task;
        executor.set_nth_result(0, false);
        queue.schedule(task, executor, NULL);
        queue.wait(task);

        UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
        CHECK(executor.nth_task(0) == &task);

        CHECK(!task.succeeded());
        CHECK(!task.cancelled());
    }
}

TEST(task_queue, schedule_and_wait_many) {
    enum { NumTasks = 20 };

    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    for (size_t n = 0; n < NumTasks; n++) {
        UNSIGNED_LONGS_EQUAL(n, executor.num_tasks());

        const bool success = (n % 3 != 0);

        TestExecutor::Task task;
        executor.set_nth_result(n, success);
        queue.schedule(task, executor, NULL);
        queue.wait(task);

        UNSIGNED_LONGS_EQUAL(n + 1, executor.num_tasks());
        CHECK(executor.nth_task(n) == &task);

        CHECK(task.succeeded() == success);
        CHECK(!task.cancelled());
    }
}

TEST(task_queue, schedule_at_one) {
    { // success
        TestExecutor executor;

        ControlTaskQueue queue;
        LONGS_EQUAL(status::StatusOK, queue.init_status());

        UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

        TestCompleter completer;
        completer.expect_success(true);
        completer.expect_cancelled(false);
        completer.expect_after(core::Millisecond);
        completer.expect_n_calls(1);

        TestExecutor::Task task;
        executor.set_nth_result(0, true);
        queue.schedule_at(task, now_plus_delay(core::Millisecond), executor, &completer);

        CHECK(completer.wait_called() == &task);

        UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
        CHECK(executor.nth_task(0) == &task);

        CHECK(task.succeeded());
        CHECK(!task.cancelled());
    }
    { // failure
        TestExecutor executor;

        ControlTaskQueue queue;
        LONGS_EQUAL(status::StatusOK, queue.init_status());

        UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

        TestCompleter completer;
        completer.expect_success(false);
        completer.expect_cancelled(false);
        completer.expect_after(core::Millisecond);
        completer.expect_n_calls(1);

        TestExecutor::Task task;
        executor.set_nth_result(0, false);
        queue.schedule_at(task, now_plus_delay(core::Millisecond), executor, &completer);

        CHECK(completer.wait_called() == &task);

        UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
        CHECK(executor.nth_task(0) == &task);

        CHECK(!task.succeeded());
        CHECK(!task.cancelled());
    }
}

TEST(task_queue, schedule_at_one_no_completer) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestExecutor::Task task;

    CHECK(!task.succeeded());
    CHECK(!task.cancelled());

    executor.set_nth_result(0, true);
    queue.schedule_at(task, now_plus_delay(core::Millisecond), executor, NULL);

    while (!task.completed()) {
        core::sleep_for(core::ClockMonotonic, core::Microsecond * 100);
    }

    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
    CHECK(executor.nth_task(0) == &task);

    CHECK(task.succeeded());
    CHECK(!task.cancelled());
}

TEST(task_queue, schedule_at_many) {
    enum { NumTasks = 20 };

    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    TestExecutor::Task tasks[NumTasks];
    TestCompleter completers[NumTasks];

    completers[0].expect_success(false);
    completers[0].expect_cancelled(false);
    completers[0].expect_n_calls(1);

    executor.block();

    executor.set_nth_result(0, false);
    queue.schedule(tasks[0], executor, &completers[0]);

    executor.wait_blocked();

    for (size_t n = 1; n < NumTasks; n++) {
        core::sleep_for(core::ClockMonotonic, core::Microsecond);

        const bool success = (n % 3 != 0);

        const core::nanoseconds_t delay =
            core::Millisecond + core::Microsecond * core::nanoseconds_t(n);

        completers[n].expect_success(success);
        completers[n].expect_cancelled(false);
        completers[n].expect_after(delay);
        completers[n].expect_n_calls(1);

        executor.set_nth_result(n, success);
        queue.schedule_at(tasks[n], now_plus_delay(delay), executor, &completers[n]);
    }

    for (size_t n = 0; n < NumTasks; n++) {
        executor.unblock_one();

        const bool success = (n % 3 != 0);

        CHECK(completers[n].wait_called() == &tasks[n]);

        UNSIGNED_LONGS_EQUAL(n + 1, executor.num_tasks());
        CHECK(executor.nth_task(n) == &tasks[n]);

        CHECK(tasks[n].succeeded() == success);
        CHECK(!tasks[n].cancelled());
    }

    executor.check_all_unblocked();
}

TEST(task_queue, schedule_at_reversed) {
    enum { NumTasks = 20 };

    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    TestExecutor::Task tasks[NumTasks];
    TestCompleter completers[NumTasks];

    completers[0].expect_success(false);
    completers[0].expect_cancelled(false);
    completers[0].expect_n_calls(1);

    executor.block();

    executor.set_nth_result(0, false);
    queue.schedule(tasks[0], executor, &completers[0]);

    executor.wait_blocked();

    const core::nanoseconds_t now = core::timestamp(core::ClockMonotonic);

    for (size_t n = 1; n < NumTasks; n++) {
        const bool success = (n % 3 != 0);

        const core::nanoseconds_t delay =
            core::Millisecond * core::nanoseconds_t(NumTasks - n);

        completers[n].expect_success(success);
        completers[n].expect_cancelled(false);
        completers[n].expect_n_calls(1);

        executor.set_nth_result(NumTasks - n, success);
        queue.schedule_at(tasks[n], now + delay, executor, &completers[n]);
    }

    executor.unblock_one();

    CHECK(completers[0].wait_called() == &tasks[0]);
    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());

    for (size_t n = 1; n < NumTasks; n++) {
        executor.unblock_one();

        const size_t idx = (NumTasks - n);

        const bool success = (idx % 3 != 0);

        CHECK(completers[idx].wait_called() == &tasks[idx]);

        UNSIGNED_LONGS_EQUAL(n + 1, executor.num_tasks());
        CHECK(executor.nth_task(n) == &tasks[idx]);

        CHECK(tasks[idx].succeeded() == success);
        CHECK(!tasks[idx].cancelled());
    }

    executor.check_all_unblocked();
}

TEST(task_queue, schedule_at_shuffled) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestCompleter completer;
    completer.expect_success(true);
    completer.expect_n_calls(4);

    TestExecutor::Task tasks[4];

    executor.set_nth_result(0, true);
    executor.set_nth_result(1, true);
    executor.set_nth_result(2, true);
    executor.set_nth_result(3, true);

    executor.block();

    const core::nanoseconds_t now = core::timestamp(core::ClockMonotonic);

    queue.schedule_at(tasks[0], now + core::Millisecond, executor, &completer);
    queue.schedule_at(tasks[1], now + core::Millisecond * 40, executor, &completer);
    queue.schedule_at(tasks[2], now + core::Millisecond * 20, executor, &completer);
    queue.schedule_at(tasks[3], now + core::Millisecond * 50, executor, &completer);

    executor.unblock_one();
    CHECK(completer.wait_called() == &tasks[0]);
    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());

    executor.unblock_one();
    CHECK(completer.wait_called() == &tasks[2]);
    UNSIGNED_LONGS_EQUAL(2, executor.num_tasks());

    executor.unblock_one();
    CHECK(completer.wait_called() == &tasks[1]);
    UNSIGNED_LONGS_EQUAL(3, executor.num_tasks());

    executor.unblock_one();
    CHECK(completer.wait_called() == &tasks[3]);
    UNSIGNED_LONGS_EQUAL(4, executor.num_tasks());

    CHECK(tasks[0].succeeded());
    CHECK(tasks[1].succeeded());
    CHECK(tasks[2].succeeded());
    CHECK(tasks[3].succeeded());

    executor.check_all_unblocked();
}

TEST(task_queue, schedule_at_same_deadline) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestCompleter completer;
    completer.expect_success(true);
    completer.expect_n_calls(4);

    TestExecutor::Task tasks[4];

    executor.set_nth_result(0, true);
    executor.set_nth_result(1, true);
    executor.set_nth_result(2, true);
    executor.set_nth_result(3, true);

    executor.block();

    const core::nanoseconds_t now = core::timestamp(core::ClockMonotonic);

    queue.schedule_at(tasks[0], now + core::Millisecond, executor, &completer);
    queue.schedule_at(tasks[1], now + core::Millisecond * 40, executor, &completer);
    queue.schedule_at(tasks[2], now + core::Millisecond * 40, executor, &completer);
    queue.schedule_at(tasks[3], now + core::Millisecond * 20, executor, &completer);

    executor.unblock_one();
    CHECK(completer.wait_called() == &tasks[0]);
    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());

    executor.unblock_one();
    CHECK(completer.wait_called() == &tasks[3]);
    UNSIGNED_LONGS_EQUAL(2, executor.num_tasks());

    executor.unblock_one();
    CHECK(completer.wait_called() == &tasks[1]);
    UNSIGNED_LONGS_EQUAL(3, executor.num_tasks());

    executor.unblock_one();
    CHECK(completer.wait_called() == &tasks[2]);
    UNSIGNED_LONGS_EQUAL(4, executor.num_tasks());

    CHECK(tasks[0].succeeded());
    CHECK(tasks[1].succeeded());
    CHECK(tasks[2].succeeded());
    CHECK(tasks[3].succeeded());

    executor.check_all_unblocked();
}

TEST(task_queue, schedule_at_and_schedule) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestCompleter completer;
    completer.expect_success(true);
    completer.expect_n_calls(4);

    TestExecutor::Task tasks[4];

    executor.set_nth_result(0, true);
    executor.set_nth_result(1, true);
    executor.set_nth_result(2, true);
    executor.set_nth_result(3, true);

    executor.block();

    const core::nanoseconds_t now = core::timestamp(core::ClockMonotonic);

    queue.schedule(tasks[0], executor, &completer);
    queue.schedule_at(tasks[1], now + core::Millisecond * 70, executor, &completer);
    queue.schedule(tasks[2], executor, &completer);
    queue.schedule_at(tasks[3], now + core::Millisecond * 50, executor, &completer);

    executor.unblock_one();
    CHECK(completer.wait_called() == &tasks[0]);
    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());

    executor.unblock_one();
    CHECK(completer.wait_called() == &tasks[2]);
    UNSIGNED_LONGS_EQUAL(2, executor.num_tasks());

    executor.unblock_one();
    CHECK(completer.wait_called() == &tasks[3]);
    UNSIGNED_LONGS_EQUAL(3, executor.num_tasks());

    executor.unblock_one();
    CHECK(completer.wait_called() == &tasks[1]);
    UNSIGNED_LONGS_EQUAL(4, executor.num_tasks());

    CHECK(tasks[0].succeeded());
    CHECK(tasks[1].succeeded());
    CHECK(tasks[2].succeeded());
    CHECK(tasks[3].succeeded());

    executor.check_all_unblocked();
}

TEST(task_queue, schedule_and_async_cancel) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestCompleter completers[4];

    TestExecutor::Task tasks[4];

    executor.set_nth_result(0, true);
    executor.set_nth_result(1, true);
    executor.set_nth_result(2, true);
    executor.set_nth_result(3, true);

    completers[0].expect_success(true);
    completers[0].expect_cancelled(false);
    completers[0].expect_n_calls(1);

    completers[1].expect_success(true);
    completers[1].expect_cancelled(false);
    completers[1].expect_n_calls(1);

    completers[2].expect_success(false);
    completers[2].expect_cancelled(true);
    completers[2].expect_n_calls(1);

    completers[3].expect_success(true);
    completers[3].expect_cancelled(false);
    completers[3].expect_n_calls(1);

    executor.block();

    queue.schedule(tasks[0], executor, &completers[0]);
    queue.schedule(tasks[1], executor, &completers[1]);
    queue.schedule(tasks[2], executor, &completers[2]);
    queue.schedule(tasks[3], executor, &completers[3]);

    executor.wait_blocked();

    queue.async_cancel(tasks[0]);
    queue.async_cancel(tasks[2]);

    executor.unblock_one();
    CHECK(completers[0].wait_called() == &tasks[0]);
    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
    CHECK(tasks[0].succeeded());
    CHECK(!tasks[0].cancelled());

    executor.unblock_one();
    CHECK(completers[1].wait_called() == &tasks[1]);
    UNSIGNED_LONGS_EQUAL(2, executor.num_tasks());
    CHECK(tasks[1].succeeded());
    CHECK(!tasks[1].cancelled());

    CHECK(completers[2].wait_called() == &tasks[2]);
    UNSIGNED_LONGS_EQUAL(2, executor.num_tasks());
    CHECK(!tasks[2].succeeded());
    CHECK(tasks[2].cancelled());

    executor.unblock_one();
    CHECK(completers[3].wait_called() == &tasks[3]);
    UNSIGNED_LONGS_EQUAL(3, executor.num_tasks());
    CHECK(tasks[3].succeeded());
    CHECK(!tasks[3].cancelled());

    executor.check_all_unblocked();
}

TEST(task_queue, schedule_at_and_async_cancel) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestCompleter completers[4];

    TestExecutor::Task tasks[4];

    executor.set_nth_result(0, true);
    executor.set_nth_result(1, true);
    executor.set_nth_result(2, true);
    executor.set_nth_result(3, true);

    completers[0].expect_success(true);
    completers[0].expect_cancelled(false);
    completers[0].expect_n_calls(1);

    completers[1].expect_success(true);
    completers[1].expect_cancelled(false);
    completers[1].expect_n_calls(1);

    completers[2].expect_success(false);
    completers[2].expect_cancelled(true);
    completers[2].expect_n_calls(1);

    completers[3].expect_success(true);
    completers[3].expect_cancelled(false);
    completers[3].expect_n_calls(1);

    executor.block();

    const core::nanoseconds_t now = core::timestamp(core::ClockMonotonic);

    queue.schedule_at(tasks[0], now + core::Millisecond, executor, &completers[0]);
    queue.schedule_at(tasks[1], now + core::Millisecond * 40, executor, &completers[1]);
    queue.schedule_at(tasks[2], now + core::Millisecond * 20, executor, &completers[2]);
    queue.schedule_at(tasks[3], now + core::Millisecond * 50, executor, &completers[3]);

    executor.wait_blocked();

    queue.async_cancel(tasks[0]);
    queue.async_cancel(tasks[2]);

    executor.unblock_one();
    CHECK(completers[0].wait_called() == &tasks[0]);
    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
    CHECK(tasks[0].succeeded());
    CHECK(!tasks[0].cancelled());

    CHECK(completers[2].wait_called() == &tasks[2]);
    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
    CHECK(!tasks[2].succeeded());
    CHECK(tasks[2].cancelled());

    executor.unblock_one();
    CHECK(completers[1].wait_called() == &tasks[1]);
    UNSIGNED_LONGS_EQUAL(2, executor.num_tasks());
    CHECK(tasks[1].succeeded());
    CHECK(!tasks[1].cancelled());

    executor.unblock_one();
    CHECK(completers[3].wait_called() == &tasks[3]);
    UNSIGNED_LONGS_EQUAL(3, executor.num_tasks());
    CHECK(tasks[3].succeeded());
    CHECK(!tasks[3].cancelled());

    executor.check_all_unblocked();
}

TEST(task_queue, cancel_and_wait) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestCompleter completer;
    completer.expect_success(false);
    completer.expect_cancelled(true);
    completer.expect_n_calls(1);

    TestExecutor::Task task;
    executor.set_nth_result(0, true);

    queue.schedule_at(task, now_plus_delay(core::Second * 999), executor, &completer);
    queue.async_cancel(task);
    queue.wait(task);

    CHECK(!task.succeeded());
    CHECK(task.cancelled());

    CHECK(completer.wait_called() == &task);

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());
}

TEST(task_queue, cancel_already_finished) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    executor.set_nth_result(0, true);

    TestCompleter completer;
    completer.expect_success(true);
    completer.expect_cancelled(false);
    completer.expect_n_calls(1);

    TestExecutor::Task task;

    queue.schedule(task, executor, &completer);
    CHECK(completer.wait_called() == &task);

    queue.async_cancel(task);

    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
    CHECK(executor.nth_task(0) == &task);

    CHECK(task.succeeded());
    CHECK(!task.cancelled());
}

TEST(task_queue, schedule_already_finished) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    executor.set_nth_result(0, true);
    executor.set_nth_result(1, true);

    TestCompleter completer;
    completer.expect_success(true);
    completer.expect_cancelled(false);
    completer.expect_n_calls(2);

    TestExecutor::Task task;

    queue.schedule(task, executor, &completer);
    CHECK(completer.wait_called() == &task);

    queue.schedule(task, executor, &completer);
    CHECK(completer.wait_called() == &task);

    UNSIGNED_LONGS_EQUAL(2, executor.num_tasks());
    CHECK(executor.nth_task(0) == &task);
    CHECK(executor.nth_task(1) == &task);

    CHECK(task.succeeded());
    CHECK(!task.cancelled());
}

TEST(task_queue, schedule_at_cancel) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    executor.set_nth_result(0, true);
    executor.set_nth_result(1, true);

    TestCompleter completer1;
    TestCompleter completer2;

    TestExecutor::Task task1;
    TestExecutor::Task task2;

    executor.block();

    completer1.expect_success(true);
    completer1.expect_cancelled(false);
    completer1.expect_n_calls(1);

    completer2.expect_success(false);
    completer2.expect_cancelled(true);
    completer2.expect_n_calls(1);

    queue.schedule(task1, executor, &completer1);
    queue.schedule(task2, executor, &completer2);
    queue.async_cancel(task2);

    executor.unblock_one();
    CHECK(completer1.wait_called() == &task1);
    CHECK(completer2.wait_called() == &task2);

    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());

    CHECK(!task2.succeeded());
    CHECK(task2.cancelled());

    completer2.expect_success(true);
    completer2.expect_cancelled(false);
    completer2.expect_n_calls(1);

    queue.schedule(task2, executor, &completer2);

    executor.unblock_one();
    CHECK(completer2.wait_called() == &task2);

    UNSIGNED_LONGS_EQUAL(2, executor.num_tasks());

    CHECK(task2.succeeded());
    CHECK(!task2.cancelled());

    executor.check_all_unblocked();
}

TEST(task_queue, reschedule_pending) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestCompleter completer;

    TestExecutor::Task task1;
    TestExecutor::Task task2;
    TestExecutor::Task task3;

    executor.block();

    executor.set_nth_result(0, true);
    executor.set_nth_result(1, true);
    executor.set_nth_result(2, true);

    queue.schedule(task1, executor, &completer);
    queue.schedule(task2, executor, &completer);
    queue.schedule(task3, executor, &completer);

    executor.wait_blocked();

    queue.schedule_at(task2, now_plus_delay(core::Millisecond), executor, &completer);

    completer.expect_success(true);
    completer.expect_cancelled(false);
    completer.expect_n_calls(1);

    executor.unblock_one();

    CHECK(completer.wait_called() == &task1);

    completer.expect_success(true);
    completer.expect_cancelled(false);
    completer.expect_n_calls(1);

    executor.unblock_one();

    CHECK(completer.wait_called() == &task3);

    completer.expect_success(true);
    completer.expect_cancelled(false);
    completer.expect_n_calls(1);

    executor.unblock_one();

    CHECK(completer.wait_called() == &task2);

    UNSIGNED_LONGS_EQUAL(3, executor.num_tasks());

    CHECK(task1.succeeded());
    CHECK(task2.succeeded());
    CHECK(task3.succeeded());

    executor.check_all_unblocked();
}

TEST(task_queue, reschedule_processing) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestCompleter completer;

    TestExecutor::Task task;

    executor.block();

    executor.set_nth_result(0, true);
    executor.set_nth_result(1, true);

    queue.schedule(task, executor, &completer);

    executor.wait_blocked();

    queue.schedule_at(task, now_plus_delay(core::Millisecond), executor, &completer);

    completer.expect_success(true);
    completer.expect_cancelled(false);
    completer.expect_n_calls(1);

    executor.unblock_one();

    CHECK(completer.wait_called() == &task);

    completer.expect_success(true);
    completer.expect_cancelled(false);
    completer.expect_n_calls(1);

    executor.unblock_one();

    CHECK(completer.wait_called() == &task);

    CHECK(task.succeeded());
    CHECK(!task.cancelled());

    UNSIGNED_LONGS_EQUAL(2, executor.num_tasks());

    executor.check_all_unblocked();
}

TEST(task_queue, reschedule_succeeded) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestCompleter completer;

    TestExecutor::Task task;

    completer.expect_success(true);
    completer.expect_cancelled(false);
    completer.expect_n_calls(1);

    executor.set_nth_result(0, true);
    queue.schedule(task, executor, &completer);

    CHECK(completer.wait_called() == &task);

    CHECK(task.succeeded());
    CHECK(!task.cancelled());

    completer.expect_success(true);
    completer.expect_cancelled(false);
    completer.expect_after(core::Millisecond);
    completer.expect_n_calls(1);

    executor.set_nth_result(1, true);
    queue.schedule_at(task, now_plus_delay(core::Millisecond), executor, &completer);

    CHECK(completer.wait_called() == &task);

    CHECK(task.succeeded());
    CHECK(!task.cancelled());

    UNSIGNED_LONGS_EQUAL(2, executor.num_tasks());
}

TEST(task_queue, reschedule_failed) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestCompleter completer;

    TestExecutor::Task task;

    completer.expect_success(false);
    completer.expect_cancelled(false);
    completer.expect_n_calls(1);

    executor.set_nth_result(0, false);
    queue.schedule(task, executor, &completer);

    CHECK(completer.wait_called() == &task);

    CHECK(!task.succeeded());
    CHECK(!task.cancelled());

    completer.expect_success(true);
    completer.expect_cancelled(false);
    completer.expect_after(core::Millisecond);
    completer.expect_n_calls(1);

    executor.set_nth_result(1, true);
    queue.schedule_at(task, now_plus_delay(core::Millisecond), executor, &completer);

    CHECK(completer.wait_called() == &task);

    CHECK(task.succeeded());
    CHECK(!task.cancelled());

    UNSIGNED_LONGS_EQUAL(2, executor.num_tasks());
}

TEST(task_queue, reschedule_cancelled) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestCompleter completer;

    TestExecutor::Task task;

    completer.expect_success(false);
    completer.expect_cancelled(true);
    completer.expect_n_calls(1);

    executor.set_nth_result(0, true);
    queue.schedule_at(task, now_plus_delay(core::Second * 999), executor, &completer);

    queue.async_cancel(task);
    queue.wait(task);

    CHECK(completer.wait_called() == &task);

    CHECK(!task.succeeded());
    CHECK(task.cancelled());

    completer.expect_success(true);
    completer.expect_cancelled(false);
    completer.expect_after(core::Millisecond);
    completer.expect_n_calls(1);

    executor.set_nth_result(1, true);
    queue.schedule_at(task, now_plus_delay(core::Millisecond), executor, &completer);

    CHECK(completer.wait_called() == &task);

    roc_panic_if(!task.succeeded());
    CHECK(task.succeeded());
    CHECK(!task.cancelled());

    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
}

TEST(task_queue, no_starvation) {
    TestExecutor executor;

    ControlTaskQueue queue;
    LONGS_EQUAL(status::StatusOK, queue.init_status());

    enum { NumTasks = 6 };

    UNSIGNED_LONGS_EQUAL(0, executor.num_tasks());

    TestCompleter completer;
    completer.expect_success(true);
    completer.expect_n_calls(NumTasks);

    TestExecutor::Task tasks[NumTasks];

    executor.block();

    const core::nanoseconds_t now = core::timestamp(core::ClockMonotonic);
    const core::nanoseconds_t WaitTime = core::Millisecond;

    queue.schedule_at(tasks[0], now + WaitTime, executor, &completer);
    queue.schedule_at(tasks[1], now + WaitTime * 2, executor, &completer);
    queue.schedule_at(tasks[2], now + WaitTime * 3, executor, &completer);
    queue.schedule(tasks[3], executor, &completer);
    queue.schedule(tasks[4], executor, &completer);
    queue.schedule(tasks[5], executor, &completer);

    for (size_t i = 0; i < NumTasks; i++) {
        executor.set_nth_result(i, true);
    }

    // wait for sleeping task to sync
    core::sleep_for(core::ClockMonotonic, WaitTime * (NumTasks / 2));

    // check that the tasks are fetched from alternating queues
    executor.unblock_one();
    ControlTask* temp = completer.wait_called();
    CHECK(temp == &tasks[0] || temp == &tasks[3]);
    UNSIGNED_LONGS_EQUAL(1, executor.num_tasks());
    CHECK(tasks[0].succeeded() || tasks[3].succeeded());

    executor.unblock_one();
    temp = completer.wait_called();
    CHECK(temp == &tasks[0] || temp == &tasks[3]);
    UNSIGNED_LONGS_EQUAL(2, executor.num_tasks());
    CHECK(tasks[0].succeeded() && tasks[3].succeeded());

    executor.unblock_one();
    temp = completer.wait_called();
    CHECK(temp == &tasks[1] || temp == &tasks[4]);
    UNSIGNED_LONGS_EQUAL(3, executor.num_tasks());
    CHECK(tasks[1].succeeded() || tasks[4].succeeded());

    executor.unblock_one();
    temp = completer.wait_called();
    CHECK(temp == &tasks[1] || temp == &tasks[4]);
    UNSIGNED_LONGS_EQUAL(4, executor.num_tasks());
    CHECK(tasks[1].succeeded() && tasks[4].succeeded());

    executor.unblock_one();
    temp = completer.wait_called();
    CHECK(temp == &tasks[2] || temp == &tasks[5]);
    UNSIGNED_LONGS_EQUAL(5, executor.num_tasks());
    CHECK(tasks[2].succeeded() || tasks[5].succeeded());

    executor.unblock_one();
    temp = completer.wait_called();
    CHECK(temp == &tasks[2] || temp == &tasks[5]);
    UNSIGNED_LONGS_EQUAL(6, executor.num_tasks());
    CHECK(tasks[2].succeeded() && tasks[5].succeeded());

    executor.check_all_unblocked();
}

} // namespace ctl
} // namespace roc
