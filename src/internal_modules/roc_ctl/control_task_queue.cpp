/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_ctl/control_task_queue.h"
#include "roc_core/cpu_instructions.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace ctl {

ControlTaskQueue::ControlTaskQueue()
    : started_(false)
    , stop_(false)
    , fetch_ready_(true)
    , ready_queue_size_(0) {
    start_thread_();
}

ControlTaskQueue::~ControlTaskQueue() {
    stop_thread_();
}

bool ControlTaskQueue::valid() const {
    return started_;
}

void ControlTaskQueue::schedule(ControlTask& task,
                                IControlTaskExecutor& executor,
                                IControlTaskCompleter* completer) {
    if (!valid()) {
        roc_panic("control task queue: attempt to use invalid queue");
    }

    if (stop_) {
        roc_panic("control task queue: attempt to use queue after stop");
    }

    setup_task_(task, executor, completer);

    renew_task_(task, 0);
}

void ControlTaskQueue::schedule_at(ControlTask& task,
                                   core::nanoseconds_t deadline,
                                   IControlTaskExecutor& executor,
                                   IControlTaskCompleter* completer) {
    if (!valid()) {
        roc_panic("control task queue: attempt to use invalid queue");
    }

    if (stop_) {
        roc_panic("control task queue: attempt to use queue after stop");
    }

    if (deadline < 0) {
        roc_panic("control task queue: deadline can't be negative");
    }

    setup_task_(task, executor, completer);

    renew_task_(task, deadline);
}

void ControlTaskQueue::async_cancel(ControlTask& task) {
    if (!valid()) {
        roc_panic("control task queue: attempt to use invalid queue");
    }

    renew_task_(task, -1);
}

void ControlTaskQueue::wait(ControlTask& task) {
    if (!valid()) {
        roc_panic("control task queue: attempt to use invalid queue");
    }

    wait_task_(task);
}

void ControlTaskQueue::run() {
    roc_log(LogDebug, "control task queue: starting event loop");

    for (;;) {
        wakeup_timer_.wait_deadline();

        if (!process_tasks_()) {
            break;
        }
    }

    roc_log(LogDebug, "control task queue: finishing event loop");
}

void ControlTaskQueue::start_thread_() {
    started_ = Thread::start();
}

void ControlTaskQueue::stop_thread_() {
    if (!started_) {
        return;
    }

    stop_ = true;
    wakeup_timer_.try_set_deadline(0);

    Thread::join();
}

bool ControlTaskQueue::process_tasks_() {
    core::Mutex::Lock lock(task_mutex_);

    for (;;) {
        ControlTask* task = NULL;

        if (fetch_ready_) {
            task = fetch_ready_task_();
            if (!task) {
                task = fetch_sleeping_task_();
            } else {
                fetch_ready_ = !fetch_ready_;
            }
        } else {
            task = fetch_sleeping_task_();
            if (!task) {
                task = fetch_ready_task_();
            } else {
                fetch_ready_ = !fetch_ready_;
            }
        }

        if (!task) {
            if (update_wakeup_timer_() == 0) {
                continue;
            }
            return !stop_;
        }

        execute_task_(*task);
    }
}

void ControlTaskQueue::setup_task_(ControlTask& task,
                                   IControlTaskExecutor& executor,
                                   IControlTaskCompleter* completer) {
    IControlTaskExecutor* prev_executor = task.executor_.exchange(&executor);
    IControlTaskCompleter* prev_completer = task.completer_.exchange(completer);

    if (prev_executor == NULL) {
        // Should not happen.
        roc_panic_if_not(prev_completer == NULL);
    } else {
        if (prev_executor != &executor) {
            roc_panic("control task queue:"
                      " attempt to reschedule task with different executor");
        }
        if (prev_completer != completer) {
            roc_panic("control task queue:"
                      " attempt to reschedule task with different completer");
        }
    }
}

void ControlTaskQueue::renew_task_(ControlTask& task, core::nanoseconds_t deadline) {
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

void ControlTaskQueue::enqueue_renewed_task_(ControlTask& task,
                                             core::nanoseconds_t deadline) {
    if (deadline < 0) {
        // We want to cancel the task and it's not sleeping, thus
        // we have nothing to do.
        if (!task.state_.compare_exchange(ControlTask::StateSleeping,
                                          ControlTask::StateReady)) {
            return;
        }
    } else {
        // Do nothing if the task is already in the ready queue.
        if (task.state_.exchange(ControlTask::StateReady) == ControlTask::StateReady) {
            return;
        }
    }

    roc_log(LogTrace,
            "control task queue: enqueueing ready task: ptr=%p renewed_deadline=%lld",
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
    // task has no completer. This is needed to ensure that the completer is
    // only called on the event loop thread because some callers may not be
    // ready for calling it in-place in async_cancel().
    if (++ready_queue_size_ == 1
        && (deadline > 0 || (deadline < 0 && !task.completer_))) {
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

bool ControlTaskQueue::try_renew_deadline_inplace_(ControlTask& task,
                                                   core::nanoseconds_t deadline) {
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

ControlTask::State ControlTaskQueue::apply_renewed_state_(ControlTask& task,
                                                          core::nanoseconds_t deadline) {
    ControlTask::State state;

    if (deadline > 0) {
        state = ControlTask::StateSleeping;
    } else if (deadline == 0) {
        state = ControlTask::StateProcessing;
    } else {
        roc_panic_if(deadline != -1);
        state = ControlTask::StateCancelling;
    }

    if (!task.state_.compare_exchange(ControlTask::StateReady, state)) {
        roc_panic("control task queue: unexpected non-ready task");
    }

    return state;
}

void ControlTaskQueue::apply_renewed_deadline_(ControlTask& task,
                                               core::nanoseconds_t deadline) {
    if (deadline >= 0) {
        reschedule_task_(task, deadline);
    } else {
        roc_panic_if_not(deadline == -1);
        cancel_task_(task);
    }
}

void ControlTaskQueue::reschedule_task_(ControlTask& task, core::nanoseconds_t deadline) {
    roc_panic_if_not(deadline >= 0);

    if (task.deadline_ == deadline) {
        return;
    }

    roc_log(LogTrace,
            "control task queue: rescheduling task: ptr=%p old_deadline=%lld "
            "new_deadline=%lld",
            (void*)&task, (long long)task.deadline_, (long long)deadline);

    if (sleeping_queue_.contains(task)) {
        remove_sleeping_task_(task);
    }

    task.deadline_ = deadline;

    if (deadline > 0) {
        insert_sleeping_task_(task);
    }
}

void ControlTaskQueue::cancel_task_(ControlTask& task) {
    roc_log(LogTrace, "control task queue: cancelling task: ptr=%p", (void*)&task);

    // This should not happend. If the task was already cancelled, its completer was
    // already called and the task may be already destroyed. The upper code should
    // prevent cancelling a task twice even if the user calls async_cancel() twice.
    if (task.deadline_ == -1) {
        roc_panic(
            "control task queue: expected pending task, got already cancelled task");
    }

    if (sleeping_queue_.contains(task)) {
        remove_sleeping_task_(task);
    }

    task.deadline_ = -1;
    task.result_ = ControlTaskCancelled;

    complete_task_(task, ControlTask::StateCancelling);
}

void ControlTaskQueue::execute_task_(ControlTask& task) {
    roc_panic_if_not(task.deadline_ >= 0);

    roc_panic_if_not(task.executor_);
    roc_panic_if_not(task.func_);

    roc_log(LogTrace, "control task queue: executing task: ptr=%p", (void*)&task);

    task.deadline_ = -1;
    task.result_ = task.executor_->execute_task(task, task.func_);

    complete_task_(task, ControlTask::StateProcessing);
}

void ControlTaskQueue::complete_task_(ControlTask& task, ControlTask::State from_state) {
    roc_panic_if_not(task.deadline_ == -1);

    IControlTaskCompleter* completer = task.completer_;

    task.state_.compare_exchange(from_state, ControlTask::StateCompleting);

    core::Semaphore* sem = task.sem_.exchange(NULL);

    if (!task.state_.compare_exchange(ControlTask::StateCompleting,
                                      ControlTask::StateCompleted)) {
        // Task was re-scheduled while we were processing it.
        // We wont mark it finished and thus wont post the semaphore this time.
        if (sem) {
            task.sem_ = sem;
            sem = NULL;
        }
    }

    // If completer and sem were NULL, we don't use task after moving to
    // StateCompleted. In this case task may be already destroyed or re-used
    // at this line.

    if (sem) {
        sem->post();
    }

    // If completer was NULL, we don't use task after posting the semaphore.
    // In this case task may be already destroyed or re-used at this line.

    if (completer) {
        completer->control_task_completed(task);
    }

    // Task may be already destroyed or re-used at this line.
}

void ControlTaskQueue::wait_task_(ControlTask& task) {
    // Nothing to do.
    if (task.state_ == ControlTask::StateCompleted) {
        return;
    }

    // Protection from concurrent waits.
    if (!task.wait_in_progress_.compare_exchange(false, true)) {
        roc_panic("control task queue: can't call wait() concurrently for the same task");
    }

    // Attach a semaphore to the task, if it's not attached yet.
    if (!task.sem_obj_) {
        task.sem_obj_.reset(new (task.sem_obj_) core::Semaphore);
    }
    task.sem_ = task.sem_obj_.get();

    // When the task is in StateCompleting, finish_task_() reads the semaphore
    // from task.sem_. Ensure that we're either before or after this block to avoid race.
    // There are only a few instructions between StateCompleting and
    // StateCompleted, so this spin loop should be very short and rare.
    while (task.state_ == ControlTask::StateCompleting) {
        core::cpu_relax();
    }

    // If the task is not in StateCompleted, it means that it's before
    // StateCompleting (because of the spin loop above), and thus finish_task_
    // will guaranteedly see the semaphore and call post, so we can safely wait on the
    // semaphore.
    //
    // If the task is in StateCompleted, and task.sem_ is NULL, it means that
    // finish_task_ successfully exchanged task.sem_ (which was non-NULL) with NULL, so it
    // will call post, so we can safely wait on the semaphore.
    //
    // Otherwise, i.e. if the task is in StateCompleted and task.sem_ is
    // non-NULL, it means that finish_task_ didn't see the semaphore and so wont call
    // post, so we should not and don't need to wait on it.
    //!
    //! This implementation is so tricky because we're attaching the semaphore only
    //! when wait() is called instead of creating it in the task constructor. This
    //! allows us to avoid an unnecessary syscall for semaphore creation (on platforms
    //! which require such a syscall) for tasks for which wait() is never called or
    //! called only after they actually finish, which is the most common case.
    if (task.state_ != ControlTask::StateCompleted || task.sem_ == NULL) {
        task.sem_obj_->wait();
    }

    task.sem_ = NULL;
    task.wait_in_progress_ = false;
}

ControlTask* ControlTaskQueue::fetch_ready_task_() {
    for (;;) {
        core::nanoseconds_t renewed_deadline;

        ControlTask* task = fetch_ready_or_renewed_task_(renewed_deadline);
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

ControlTask*
ControlTaskQueue::fetch_ready_or_renewed_task_(core::nanoseconds_t& renewed_deadline) {
    // try_pop_front_exclusive() returns NULL if queue is empty or push_back() is
    // in progress; in the later case ready_queue_size_ is guaranteed to be
    // non-zero and process_tasks_() will call us again soon.
    ControlTask* task = ready_queue_.try_pop_front_exclusive();
    if (!task) {
        roc_log(LogTrace,
                "control task queue: ready task queue is empty or being pushed");
        return NULL;
    }

    // renewed_deadline is being updated concurrently.
    // Re-add task to the queue to try again later.
    if (!task->renewed_deadline_.try_load(renewed_deadline)) {
        roc_log(
            LogTrace,
            "control task queue: re-adding task to ready queue after first read: ptr=%p",
            (void*)task);

        ready_queue_.push_back(*task);
        return NULL;
    }

    // Switch task state based on the renewed deadline.
    const ControlTask::State new_state = apply_renewed_state_(*task, renewed_deadline);

    // Catch the rare situation where renewed_deadline was updated after we've
    // read it above, but before we switched the task state above. In this case,
    // renew_task_() will think that the task is already added to ready_queue_
    // and will just return, so we should re-enqueue it by ourselves.
    core::nanoseconds_t new_renewed_deadline;
    if (!task->renewed_deadline_.try_load(new_renewed_deadline)
        || renewed_deadline != new_renewed_deadline) {
        roc_log(
            LogTrace,
            "control task queue: re-adding task to ready queue after second read: ptr=%p",
            (void*)task);

        if (task->state_.compare_exchange(new_state, ControlTask::StateReady)) {
            ready_queue_.push_back(*task);
        } else {
            --ready_queue_size_;
        }
        return NULL;
    }

    // The task was removed and wasn't re-added.
    --ready_queue_size_;

    roc_log(LogTrace, "control task queue: fetching ready task: ptr=%p", (void*)task);

    return task;
}

ControlTask* ControlTaskQueue::fetch_sleeping_task_() {
    ControlTask* task = sleeping_queue_.front();
    if (!task) {
        return NULL;
    }

    if (task->deadline_ > core::timestamp()) {
        return NULL;
    }

    roc_log(LogTrace, "control task queue: fetching sleeping task: ptr=%p deadline=%lld",
            (void*)task, (long long)task->deadline_);

    remove_sleeping_task_(*task);

    if (!task->state_.compare_exchange(ControlTask::StateSleeping,
                                       ControlTask::StateProcessing)) {
        return NULL;
    }

    return task;
}

void ControlTaskQueue::insert_sleeping_task_(ControlTask& task) {
    roc_panic_if_not(task.deadline_ > 0);

    ControlTask* pos = sleeping_queue_.front();

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

void ControlTaskQueue::remove_sleeping_task_(ControlTask& task) {
    roc_panic_if_not(task.deadline_ > 0);

    sleeping_queue_.remove(task);
}

core::nanoseconds_t ControlTaskQueue::update_wakeup_timer_() {
    core::nanoseconds_t deadline = 0;

    if (ready_queue_size_ == 0) {
        if (ControlTask* task = sleeping_queue_.front()) {
            deadline = task->deadline_;
        } else {
            deadline = -1;
        }
    }

    roc_log(LogTrace, "control task queue: updating wakeup deadline: deadline=%lld",
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
