/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_ctl/task_queue.h"
#include "roc_core/cpu_ops.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace ctl {

TaskQueue::Task::Task()
    : state_(StateFinished)
    , result_(TaskFailed)
    , deadline_(0)
    , renewed_deadline_(0)
    , renew_in_progress_(false)
    , wait_in_progress_(false)
    , handler_(NULL)
    , sem_(NULL) {
}

TaskQueue::Task::~Task() {
    if (state_ != StateFinished) {
        roc_panic("task queue: attempt to destroy task before it's finished");
    }
}

bool TaskQueue::Task::pending() const {
    return state_ != StateFinished;
}

bool TaskQueue::Task::success() const {
    return result_ == TaskSucceeded;
}

bool TaskQueue::Task::cancelled() const {
    return result_ == TaskCancelled;
}

TaskQueue::ICompletionHandler::~ICompletionHandler() {
}

TaskQueue::TaskQueue()
    : started_(false)
    , stop_(false)
    , ready_queue_size_(0) {
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
    if (!valid()) {
        roc_panic("task queue: attempt to use invalid queue");
    }

    if (stop_) {
        roc_panic("task queue: attempt to use queue after stop_and_wait()");
    }

    initialize_task_(task, handler);

    renew_task_(task, 0);
}

void TaskQueue::schedule_at(Task& task,
                            core::nanoseconds_t deadline,
                            ICompletionHandler* handler) {
    if (!valid()) {
        roc_panic("task queue: attempt to use invalid queue");
    }

    if (stop_) {
        roc_panic("task queue: attempt to use queue after stop_and_wait()");
    }

    if (deadline < 0) {
        roc_panic("task queue: deadline can't be negative");
    }

    initialize_task_(task, handler);

    renew_task_(task, deadline);
}

void TaskQueue::reschedule_at(Task& task, core::nanoseconds_t deadline) {
    if (!valid()) {
        roc_panic("task queue: attempt to use invalid queue");
    }

    if (stop_) {
        roc_panic("task queue: attempt to use queue after stop_and_wait()");
    }

    if (deadline < 0) {
        roc_panic("task queue: deadline can't be negative");
    }

    renew_task_(task, deadline);
}

void TaskQueue::async_cancel(Task& task) {
    if (!valid()) {
        roc_panic("task queue: attempt to use invalid queue");
    }

    renew_task_(task, -1);
}

void TaskQueue::wait(Task& task) {
    if (!valid()) {
        roc_panic("task queue: attempt to use invalid queue");
    }

    wait_task_(task);
}

void TaskQueue::run() {
    roc_log(LogDebug, "task queue: starting event loop");

    for (;;) {
        wakeup_timer_.wait_deadline();

        if (!process_tasks_()) {
            break;
        }
    }

    roc_log(LogDebug, "task queue: finishing event loop");
}

void TaskQueue::stop_and_wait() {
    if (!started_) {
        return;
    }

    stop_ = true;
    wakeup_timer_.try_set_deadline(0);

    Thread::join();
}

bool TaskQueue::process_tasks_() {
    core::Mutex::Lock lock(task_mutex_);

    for (;;) {
        Task* task = fetch_ready_task_();

        if (!task) {
            task = fetch_sleeping_task_();
        }

        if (!task) {
            if (update_wakeup_timer_() == 0) {
                continue;
            }
            return !stop_;
        }

        process_task_(*task);
    }
}

void TaskQueue::initialize_task_(Task& task, ICompletionHandler* handler) {
    if (!task.state_.compare_exchange(StateFinished, StateInitializing)) {
        roc_panic("task queue: attempt to schedule task again before it finished");
    }

    task.handler_ = handler;
}

void TaskQueue::renew_task_(Task& task, core::nanoseconds_t deadline) {
    // Handle concurrent task updates.
    // If there are concurrent reschedule_after() and/or async_cancel() calls, only one
    // of them wins, and other give up and do nothing. This is okay, since if they were
    // serialized, only one of them (the last one) would take effect.
    // In addition, if async_cancel() sees that the task is not in StatePending, it also
    // does nothing, since the task will be anyway executed or cancelled very soon.
    if (!task.renew_in_progress_.compare_exchange(false, true)) {
        return;
    }

    // Set the new desired deadline.
    // Allowed deadline values are:
    //  positive - schedule task at the given point of time
    //  "0" - process task as soon as possible
    //  "-1" - cancel the task
    // The new deadline will be applied either by try_renew_deadline_inplace_()
    // in this thread, or by fetch_ready_task_() later in event loop thread.
    task.renewed_deadline_.exclusive_store(deadline);

    // Add task to the ready queue (if it's not there already), to be renewed
    // later. Or maybe renew it in-place if possible.
    enqueue_renewed_task_(task, deadline);

    // Finish operation.
    task.renew_in_progress_ = false;
}

void TaskQueue::enqueue_renewed_task_(Task& task, core::nanoseconds_t deadline) {
    if (deadline < 0) {
        // We want to cancel the task and it's not sleeping, thus
        // we have nothing to do.
        if (!task.state_.compare_exchange(StateSleeping, StateReady)) {
            return;
        }
    } else {
        // Do nothing if the task is already in the ready queue.
        if (task.state_.exchange(StateReady) == StateReady) {
            return;
        }
    }

    roc_log(LogTrace, "task queue: enqueueing ready task: ptr=%p renewed_deadline=%lld",
            (void*)&task, (long long)deadline);

    // If we don't want to process the task imemdiately, i.e. we want to cancel it
    // or just change deadline, there is no need to wake up the event loop thread
    // if it is sleeping currently.
    //
    // So, if the ready_queue_ queue is empty and the mutex is free, which means
    // that the event loop thread is likely sleeping, we cancel or update the task
    // in-place, without adding it to the queue and waking up the event loop thread,
    // thus avoiding an unnecessary thread switch.
    //
    // If we're cancelling the task, this optimization is performed only if the
    // task has no completion handler. This is needed to ensure that the handler
    // is only called on the event loop thread because some callers may not be
    // ready for calling it in-place in async_cancel().
    if (++ready_queue_size_ == 1 && (deadline > 0 || (deadline < 0 && !task.handler_))) {
        if (try_renew_deadline_inplace_(task, deadline)) {
            return;
        }
    }

    // Add task to the ready queue.
    ready_queue_.push_back(task);

    // Wake up event loop thread.
    // This wakeup will either succeed or handled by concurrent call to
    // update_wakeup_timer_().
    wakeup_timer_.try_set_deadline(0);
}

bool TaskQueue::try_renew_deadline_inplace_(Task& task, core::nanoseconds_t deadline) {
    roc_panic_if(deadline == 0);

    if (!task_mutex_.try_lock()) {
        return false;
    }

    apply_renewed_state_(task, deadline);
    apply_renewed_deadline_(task, deadline);

    --ready_queue_size_;
    update_wakeup_timer_();

    task_mutex_.unlock();

    return true;
}

TaskQueue::TaskState TaskQueue::apply_renewed_state_(Task& task,
                                                     core::nanoseconds_t deadline) {
    TaskState state;

    if (deadline > 0) {
        state = StateSleeping;
    } else if (deadline == 0) {
        state = StateProcessing;
    } else {
        roc_panic_if(deadline != -1);
        state = StateCancelling;
    }

    if (!task.state_.compare_exchange(StateReady, state)) {
        roc_panic("task queue: unexpected non-ready task");
    }

    return state;
}

void TaskQueue::apply_renewed_deadline_(Task& task, core::nanoseconds_t deadline) {
    if (deadline >= 0) {
        reschedule_task_(task, deadline);
    } else {
        roc_panic_if_not(deadline == -1);
        cancel_task_(task);
    }
}

void TaskQueue::reschedule_task_(Task& task, core::nanoseconds_t deadline) {
    roc_panic_if_not(deadline >= 0);

    if (task.deadline_ == deadline) {
        return;
    }

    roc_log(LogTrace,
            "task queue: rescheduling task: ptr=%p old_deadline=%lld new_deadline=%lld",
            (void*)&task, (long long)task.deadline_, (long long)deadline);

    if (sleeping_queue_.contains(task)) {
        remove_sleeping_task_(task);
    }

    task.deadline_ = deadline;

    if (deadline > 0) {
        insert_sleeping_task_(task);
    }
}

void TaskQueue::cancel_task_(Task& task) {
    roc_log(LogTrace, "task queue: cancelling task: ptr=%p", (void*)&task);

    // This should not happend. If the task was already cancelled, its handler was
    // already called and the task may be already destroyed. The upper code should
    // prevent cancelling a task twice even if the user calls async_cancel() twice.
    if (task.deadline_ == -1) {
        roc_panic("task queue: expected pending task, got already cancelled task");
    }

    if (sleeping_queue_.contains(task)) {
        remove_sleeping_task_(task);
    }

    task.deadline_ = -1;
    task.result_ = TaskCancelled;

    finish_task_(task, StateCancelling);
}

void TaskQueue::process_task_(Task& task) {
    roc_panic_if_not(task.deadline_ >= 0);

    roc_log(LogTrace, "task queue: processing task: ptr=%p", (void*)&task);

    task.deadline_ = -1;
    task.result_ = process_task_imp(task);

    finish_task_(task, StateProcessing);
}

void TaskQueue::finish_task_(Task& task, TaskState from_state) {
    roc_panic_if_not(task.deadline_ == -1);

    ICompletionHandler* handler = task.handler_;

    task.state_.compare_exchange(from_state, StateFinishing);

    core::Semaphore* sem = task.sem_.exchange(NULL);

    if (!task.state_.compare_exchange(StateFinishing, StateFinished)) {
        // Task was re-scheduled while we were processing it.
        // We wont mark it finished and thus wont post the semaphore this time.
        if (sem) {
            task.sem_ = sem;
            sem = NULL;
        }
    }

    // If handler and sem were NULL, we don't use task after moving to StateFinished.
    // In this case task may be already destroyed or re-used at this line.

    if (sem) {
        sem->post();
    }

    // If handler was NULL, we don't use task after posting the semaphore.
    // In this case task may be already destroyed or re-used at this line.

    if (handler) {
        handler->control_task_finished(task);
    }

    // Task may be already destroyed or re-used at this line.
}

void TaskQueue::wait_task_(Task& task) {
    // Nothing to do.
    if (task.state_ == StateFinished) {
        return;
    }

    // Protection from concurrent waits.
    if (!task.wait_in_progress_.compare_exchange(false, true)) {
        roc_panic("task queue: can't call wait() concurrently for the same task");
    }

    // Attach a semaphore to the task, if it's not attached yet.
    if (!task.sem_obj_) {
        task.sem_obj_.reset(new (task.sem_obj_) core::Semaphore);
    }
    task.sem_ = task.sem_obj_.get();

    // When the task is in StateFinishing, finish_task_() reads the semaphore from
    // task.sem_. Ensure that we're either before or after this block to avoid race.
    // There are only a few instructions between StateFinishing and StateFinished,
    // so this spin loop should very short and rare.
    while (task.state_ == StateFinishing) {
        core::cpu_relax();
    }

    // If the task is not in StateFinished, it means that it's before StateFinishing
    // (because of the spin loop above), and thus finish_task_ will guaranteedly see
    // the semaphore and call post, so we can safely wait on the semaphore.
    //
    // If the task is in StateFinished, and task.sem_ is NULL, it means that finish_task_
    // successfully exchanged task.sem_ (which was non-NULL) with NULL, so it will call
    // post, so we can safely wait on the semaphore.
    //
    // Otherwise, i.e. if the task is in StateFinished and task.sem_ is non-NULL, it
    // means that finish_task_ didn't see the semaphore and so wont call post, so we
    // should not and don't need to wait on it.
    //!
    //! This implementation is so tricky because we're attaching the semaphore only
    //! when wait() is called instead of creating it in the task constructor. This
    //! allows us to avoid an unnecessary syscall for semaphore creation (on platforms
    //! which require such a syscall) for tasks for which wait() is never called or
    //! called only after they actually finish, which is the most common case.
    if (task.state_ != StateFinished || task.sem_ == NULL) {
        task.sem_obj_->wait();
    }

    task.sem_ = NULL;
    task.wait_in_progress_ = false;
}

TaskQueue::Task* TaskQueue::fetch_ready_task_() {
    for (;;) {
        core::nanoseconds_t renewed_deadline;

        Task* task = fetch_ready_or_renewed_task_(renewed_deadline);
        if (!task) {
            return NULL;
        }

        apply_renewed_deadline_(*task, renewed_deadline);

        // This task should not be processed, it was added to ready queue
        // just to update its deadline.
        if (renewed_deadline != 0) {
            continue;
        }

        roc_panic_if_not(task->deadline_ == 0);
        return task;
    }
}

TaskQueue::Task*
TaskQueue::fetch_ready_or_renewed_task_(core::nanoseconds_t& renewed_deadline) {
    // try_pop_front_exclusive() returns NULL if queue is empty or push_back() is
    // in progress; in the later case ready_queue_size_ is guaranteed to be
    // non-zero and process_tasks_() will call us again soon.
    Task* task = ready_queue_.try_pop_front_exclusive();
    if (!task) {
        roc_log(LogTrace, "task queue: ready task queue is empty or being pushed");
        return NULL;
    }

    // renewed_deadline is being updated concurrently.
    // Re-add task to the queue to try again later.
    if (!task->renewed_deadline_.try_load(renewed_deadline)) {
        roc_log(LogTrace,
                "task queue: re-adding task to ready queue after first read: ptr=%p",
                (void*)task);

        ready_queue_.push_back(*task);
        return NULL;
    }

    // Switch task state based on the renewed deadline.
    const TaskState new_state = apply_renewed_state_(*task, renewed_deadline);

    // Catch the rare situation where renewed_deadline was updated after we've
    // read it above, but before we switched the task state above. In this case,
    // renew_task_() will think that the task is already added to ready_queue_
    // and will just return, so we should re-enqueue it by ourselves.
    core::nanoseconds_t new_renewed_deadline;
    if (!task->renewed_deadline_.try_load(new_renewed_deadline)
        || renewed_deadline != new_renewed_deadline) {
        roc_log(LogTrace,
                "task queue: re-adding task to ready queue after second read: ptr=%p",
                (void*)task);

        if (task->state_.compare_exchange(new_state, StateReady)) {
            ready_queue_.push_back(*task);
        } else {
            --ready_queue_size_;
        }
        return NULL;
    }

    // The task was removed and wasn't re-added.
    --ready_queue_size_;

    roc_log(LogTrace, "task queue: fetching ready task: ptr=%p", (void*)task);

    return task;
}

TaskQueue::Task* TaskQueue::fetch_sleeping_task_() {
    Task* task = sleeping_queue_.front();
    if (!task) {
        return NULL;
    }

    if (task->deadline_ > core::timestamp()) {
        return NULL;
    }

    roc_log(LogTrace, "task queue: fetching sleeping task: ptr=%p deadline=%lld",
            (void*)task, (long long)task->deadline_);

    remove_sleeping_task_(*task);

    if (!task->state_.compare_exchange(StateSleeping, StateProcessing)) {
        return NULL;
    }

    return task;
}

void TaskQueue::insert_sleeping_task_(Task& task) {
    roc_panic_if_not(task.deadline_ > 0);

    Task* pos = sleeping_queue_.front();

    for (; pos; pos = sleeping_queue_.nextof(*pos)) {
        if (pos->deadline_ > task.deadline_) {
            break;
        }
    }

    if (pos) {
        sleeping_queue_.insert_before(task, *pos);
    } else {
        sleeping_queue_.push_back(task);
    }
}

void TaskQueue::remove_sleeping_task_(Task& task) {
    roc_panic_if_not(task.deadline_ > 0);

    sleeping_queue_.remove(task);
}

core::nanoseconds_t TaskQueue::update_wakeup_timer_() {
    core::nanoseconds_t deadline = 0;

    if (ready_queue_size_ == 0) {
        if (Task* task = sleeping_queue_.front()) {
            deadline = task->deadline_;
        } else {
            deadline = -1;
        }
    }

    roc_log(LogTrace, "task queue: updating wakeup deadline: deadline=%lld",
            (long long)deadline);

    wakeup_timer_.try_set_deadline(deadline);

    // We should check whether new tasks were added while we were updating the timer.
    // In this case, try_set_deadline(0) in renew_task_() was probably failed, and
    // we should call it by ourselves to wake up the event loop thread.
    if (deadline != 0 && ready_queue_size_ != 0) {
        deadline = 0;
        wakeup_timer_.try_set_deadline(0);
    }

    return deadline;
}

} // namespace ctl
} // namespace roc
