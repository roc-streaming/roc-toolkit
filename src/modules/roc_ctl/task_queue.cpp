/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_ctl/task_queue.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace ctl {

TaskQueue::Task::Task() {
    set_scheduling_params_(0, NULL);
    reset_state_(false);
}

TaskQueue::Task::~Task() {
    if (pending_) {
        roc_panic("task queue: attemp to destroy task before it's finished");
    }
}

bool TaskQueue::Task::cancelled() const {
    return result_ == TaskCancelled;
}

bool TaskQueue::Task::success() const {
    return result_ == TaskSucceeded;
}

void TaskQueue::Task::set_scheduling_params_(core::nanoseconds_t delay,
                                             ICompletionHandler* handler) {
    if (pending_) {
        roc_panic("task queue: attempt to re-schedule task before finishing it");
    }

    set_deadline_(delay);

    handler_ = handler;
}

void TaskQueue::Task::set_deadline_(core::nanoseconds_t delay) {
    if (delay < 0) {
        roc_panic("task queue: delay can't be negative");
    }

    if (delay) {
        deadline_ = core::timestamp() + delay;
    } else {
        deadline_ = 0;
    }
}

void TaskQueue::Task::reset_state_(bool pending) {
    pending_ = pending;
    result_ = TaskFailed;
    request_cancel_ = false;
}

TaskQueue::ICompletionHandler::~ICompletionHandler() {
}

TaskQueue::TaskQueue()
    : started_(false)
    , stop_(false)
    , request_reschedule_(false)
    , first_task_with_deadline_(NULL)
    , currently_processing_task_(NULL)
    , finished_cond_(mutex_) {
    started_ = Thread::start();
}

TaskQueue::~TaskQueue() {
    if (joinable()) {
        roc_panic(
            "task queue: attempt to call destructor before calling stop_and_wait()");
    }
}

bool TaskQueue::valid() const {
    return started_;
}

void TaskQueue::schedule(Task& task, ICompletionHandler* handler) {
    schedule_after(task, 0, handler);
}

void TaskQueue::schedule_after(Task& task,
                               core::nanoseconds_t delay,
                               ICompletionHandler* handler) {
    core::Mutex::Lock lock(mutex_);

    if (!valid()) {
        roc_panic("task queue: attempt to use invalid loop");
    }

    task.set_scheduling_params_(delay, handler);

    schedule_task_(task);
}

void TaskQueue::reschedule_after(Task& task, core::nanoseconds_t delay) {
    core::Mutex::Lock lock(mutex_);

    if (!valid()) {
        roc_panic("task queue: attempt to use invalid loop");
    }

    reschedule_task_(task, delay);
}

bool TaskQueue::schedule_and_wait(Task& task) {
    core::Mutex::Lock lock(mutex_);

    if (!valid()) {
        roc_panic("task queue: attempt to use invalid loop");
    }

    task.set_scheduling_params_(0, NULL);

    schedule_task_(task);

    while (task.pending_) {
        finished_cond_.wait();
    }

    return task.success();
}

void TaskQueue::async_cancel(Task& task) {
    core::Mutex::Lock lock(mutex_);

    if (!valid()) {
        roc_panic("task queue: attempt to use invalid loop");
    }

    cancel_task_(task);
}

void TaskQueue::cancel_and_wait(Task& task) {
    core::Mutex::Lock lock(mutex_);

    if (!valid()) {
        roc_panic("task queue: attempt to use invalid loop");
    }

    cancel_task_(task);

    while (task.pending_) {
        finished_cond_.wait();
    }
}

void TaskQueue::run() {
    roc_log(LogDebug, "task queue: starting event loop");

    for (;;) {
        wakeup_timer_.wait_deadline();

        Task* task = begin_task_processing_();

        if (!task && stop_) {
            break;
        }

        if (!task) {
            continue;
        }

        process_task_(*task);

        end_task_processing_();
    }

    roc_log(LogDebug, "task queue: finishing event loop");
}

void TaskQueue::stop_and_wait() {
    if (!started_) {
        return;
    }

    {
        core::Mutex::Lock lock(mutex_);

        if (pending_tasks_.size() != 0) {
            roc_panic(
                "task queue: attempt to call stop_and_wait() before finishing all tasks");
        }

        stop_ = true;
    }

    wakeup_timer_.set_deadline(0);

    Thread::join();
}

TaskQueue::Task* TaskQueue::begin_task_processing_() {
    core::Mutex::Lock lock(mutex_);

    Task* task = pending_tasks_.front();

    if (!task || task->deadline_ > core::timestamp()) {
        // spurious wake up
        update_next_deadline_();
        return NULL;
    }

    if (first_task_with_deadline_ == task) {
        first_task_with_deadline_ = pending_tasks_.nextof(*task);
    }

    pending_tasks_.remove(*task);
    update_next_deadline_();

    currently_processing_task_ = task;

    return task;
}

void TaskQueue::process_task_(Task& task) {
    ICompletionHandler* handler = task.handler_;

    if (task.request_cancel_) {
        roc_log(LogTrace, "task queue: cancelling task: ptr=%p", (void*)&task);
        task.result_ = TaskCancelled;
    } else {
        roc_log(LogTrace, "task queue: processing task: ptr=%p", (void*)&task);
        task.result_ = process_task_imp(task);
    }

    task.pending_ = false;

    if (handler) {
        handler->control_task_finished(task);
    }
}

void TaskQueue::end_task_processing_() {
    core::Mutex::Lock lock(mutex_);

    finished_cond_.broadcast();

    if (request_reschedule_) {
        request_reschedule_ = false;
        schedule_task_(*currently_processing_task_);
    }

    currently_processing_task_ = NULL;
}

void TaskQueue::schedule_task_(Task& task) {
    if (stop_) {
        roc_panic("task queue: attempt to schedule task after calling stop_and_wait()");
    }

    task.reset_state_(true);

    roc_log(LogTrace, "task queue: enqueuing task: ptr=%p deadline=%lld", (void*)&task,
            (long long)task.deadline_);

    add_to_pending_(task);
    update_next_deadline_();
}

void TaskQueue::reschedule_task_(Task& task, core::nanoseconds_t delay) {
    if (stop_) {
        roc_panic("task queue: attempt to reschedule task after calling stop_and_wait()");
    }

    roc_log(LogTrace, "task queue: rescheduling task: ptr=%p", (void*)&task);

    if (pending_tasks_.contains(task)) {
        remove_from_pending_(task);
        task.set_deadline_(delay);
        schedule_task_(task);
    } else if (currently_processing_task_ == &task) {
        task.set_deadline_(delay);
        request_reschedule_ = true;
    } else {
        task.set_deadline_(delay);
        schedule_task_(task);
    }
}

void TaskQueue::cancel_task_(Task& task) {
    if (stop_) {
        roc_panic("task queue: attempt to cancel task after calling stop_and_wait()");
    }

    if (!pending_tasks_.contains(task)) {
        return;
    }

    roc_log(LogTrace, "task queue: requesting to cancel task: ptr=%p", (void*)&task);

    task.request_cancel_ = true;

    if (task.deadline_ != 0) {
        remove_from_pending_(task);
        task.set_deadline_(0);
        add_to_pending_(task);
        update_next_deadline_();
    }
}

void TaskQueue::add_to_pending_(Task& task) {
    Task* pos = first_task_with_deadline_;

    for (; pos; pos = pending_tasks_.nextof(*pos)) {
        if (pos->deadline_ > task.deadline_) {
            break;
        }
    }

    if (pos) {
        pending_tasks_.insert_before(task, *pos);
    } else {
        pending_tasks_.push_back(task);
    }

    if (first_task_with_deadline_ == pos && task.deadline_) {
        first_task_with_deadline_ = &task;
    }
}

void TaskQueue::remove_from_pending_(Task& task) {
    if (first_task_with_deadline_ == &task) {
        first_task_with_deadline_ = pending_tasks_.nextof(task);
    }

    pending_tasks_.remove(task);
}

void TaskQueue::update_next_deadline_() {
    core::nanoseconds_t deadline = 0;

    if (Task* task = pending_tasks_.front()) {
        deadline = task->deadline_;
    } else {
        deadline = -1;
    }

    roc_log(LogTrace, "task queue: updating deadline: deadline=%lld ftwd_ptr=%p",
            (long long)deadline, (void*)first_task_with_deadline_);

    wakeup_timer_.set_deadline(deadline);
}

} // namespace ctl
} // namespace roc
