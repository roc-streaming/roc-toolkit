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
    , ready_queue_size_(0)
    , init_status_(status::NoStatus) {
    roc_log(LogTrace, "control task queue: starting thread");

    if (!start_thread_()) {
        init_status_ = status::StatusErrThread;
        return;
    }

    init_status_ = status::StatusOK;
}

ControlTaskQueue::~ControlTaskQueue() {
    roc_log(LogTrace, "control task queue: stopping thread");

    stop_thread_();
}

status::StatusCode ControlTaskQueue::init_status() const {
    return init_status_;
}

void ControlTaskQueue::schedule(ControlTask& task,
                                IControlTaskExecutor& executor,
                                IControlTaskCompleter* completer) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (stop_) {
        roc_panic("control task queue: attempt to use queue after stop");
    }

    setup_task_(task, executor, completer);

    request_renew_(task, 0);
}

void ControlTaskQueue::schedule_at(ControlTask& task,
                                   core::nanoseconds_t deadline,
                                   IControlTaskExecutor& executor,
                                   IControlTaskCompleter* completer) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (stop_) {
        roc_panic("control task queue: attempt to use queue after stop");
    }

    if (deadline < 0) {
        roc_panic("control task queue: deadline can't be negative");
    }

    setup_task_(task, executor, completer);

    request_renew_(task, deadline);
}

void ControlTaskQueue::resume(ControlTask& task) {
    roc_panic_if(init_status_ != status::StatusOK);

    request_resume_(task);
}

void ControlTaskQueue::async_cancel(ControlTask& task) {
    roc_panic_if(init_status_ != status::StatusOK);

    request_renew_(task, -1);
}

void ControlTaskQueue::wait(ControlTask& task) {
    roc_panic_if(init_status_ != status::StatusOK);

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

bool ControlTaskQueue::start_thread_() {
    return (started_ = Thread::start());
}

void ControlTaskQueue::stop_thread_() {
    if (!started_) {
        return;
    }

    stop_ = true;
    wakeup_timer_.try_set_deadline(0);

    Thread::join();
}

void ControlTaskQueue::setup_task_(ControlTask& task,
                                   IControlTaskExecutor& executor,
                                   IControlTaskCompleter* completer) {
    if (task.executor_ == NULL) {
        roc_panic_if_not(task.completer_ == NULL);

        task.executor_ = &executor;
        task.completer_ = completer;
    } else {
        if (task.executor_ != &executor) {
            roc_panic("control task queue:"
                      " attempt to reschedule task with different executor: ptr=%p",
                      (void*)&task);
        }
        if (task.completer_ != completer) {
            roc_panic("control task queue:"
                      " attempt to reschedule task with different completer: ptr=%p",
                      (void*)&task);
        }
    }
}

void ControlTaskQueue::request_resume_(ControlTask& task) {
    // If the task is already being resumed, do nothing.
    // Otherwise, mark task as being resumed.
    const unsigned task_flags = task.flags_.fetch_or(ControlTask::FlagResumed);

    // Catch bugs.
    ControlTask::validate_flags(task_flags);

    if (task_flags & ControlTask::FlagResumed) {
        return;
    }

    // If the task is already in the ready queue, do nothing.
    // Otherwise, place task to the ready queue.
    if (task.state_.exchange(ControlTask::StateReady) == ControlTask::StateReady) {
        return;
    }

    // First commit new queue size.
    ++ready_queue_size_;

    // Add task to the ready queue.
    ready_queue_.push_back(task);

    // Wake up event loop thread.
    // This wakeup will either succeed or handled by concurrent call to
    // update_wakeup_timer_().
    wakeup_timer_.try_set_deadline(0);
}

void ControlTaskQueue::request_renew_(ControlTask& task, core::nanoseconds_t deadline) {
    // Cut off concurrent task renewals. This simplifies implementation.
    // If there are concurrent schedule and/or async_cancel calls, only one of them
    // wins, and other give up and do nothing. This is okay, since if they were
    // serialized, only one of them (the last one) would take effect.
    if (!task.renew_guard_.compare_exchange(false, true)) {
        return;
    }

    request_renew_guarded_(task, deadline);

    // Finish operation.
    task.renew_guard_ = false;
}

void ControlTaskQueue::request_renew_guarded_(ControlTask& task,
                                              core::nanoseconds_t deadline) {
    // Set the new desired deadline.
    // Allowed deadline values are:
    //  positive - schedule task at the given point of time
    //  "0" - process task as soon as possible
    //  "-1" - cancel the task
    // The new deadline will be applied either by try_renew_inplace_()
    // in this thread, or by fetch_ready_task_() later in event loop thread.
    core::seqlock_version_t version = 0;
    task.renewed_deadline_.exclusive_store_v(deadline, version);

    // Catch bugs.
    ControlTask::validate_deadline(deadline, version);

    if (deadline < 0) {
        // We want to cancel the task and it's not sleeping, thus we have nothing to do:
        // if it's not pausing, it will be anyway completed soon, if it's pausing, it
        // will check renewed deadline before going to sleep.
        if (!task.state_.compare_exchange(ControlTask::StateSleeping,
                                          ControlTask::StateReady)) {
            return;
        }
    } else {
        // Do nothing if the task is paused.
        // After the task resumes and completes, it will find out that
        // FlagRescheduleRequested was set and handle the renewed deadline.
        const unsigned task_flags = task.flags_;

        // Catch bugs.
        ControlTask::validate_flags(task_flags);

        if (task_flags & ControlTask::FlagPaused) {
            return;
        }

        // Do nothing if the task is already in the ready queue.
        if (task.state_.exchange(ControlTask::StateReady) == ControlTask::StateReady) {
            return;
        }
    }

    roc_log(LogTrace,
            "control task queue: enqueueing ready task:"
            " ptr=%p renewed_deadline=%lld renewed_version=%llu",
            (void*)&task, (long long)deadline, (unsigned long long)version);

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
        if (try_renew_inplace_(task, deadline, version)) {
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

bool ControlTaskQueue::try_renew_inplace_(ControlTask& task,
                                          core::nanoseconds_t deadline,
                                          core::seqlock_version_t version) {
    roc_panic_if(deadline == 0);

    // Try to obtain lock.
    // This succeeds if the event loop thread sleeps.
    if (!task_mutex_.try_lock()) {
        return false;
    }

    // Read task flags after mutex is locked.
    // During the lock is held, only resume flag may be set concurrently.
    const unsigned task_flags = task.flags_;

    // Ensure that the task is either not paused, or we're going to cancel it. These are
    // the only cases when it's legit to renew the task without waking up the even loop
    // thread. This bool may evaluate to false only if the pause flag was set in a short
    // period after we checked it in request_renew_guarded_() and before we locked mutex.
    const bool can_renew_inplace =
        !(task_flags & ControlTask::FlagPaused) || (deadline < 0 && !task.completer_);

    if (can_renew_inplace) {
        roc_log(LogTrace,
                "control task queue: renewing task in-place:"
                " ptr=%p renewed_deadline=%lld renewed_version=%llu",
                (void*)&task, (long long)deadline, (unsigned long long)version);

        renew_state_(task, task_flags, deadline);
        renew_scheduling_(task, task_flags, deadline, version);

        --ready_queue_size_;
        update_wakeup_timer_();
    }

    task_mutex_.unlock();

    return can_renew_inplace;
}

ControlTask::State ControlTaskQueue::renew_state_(ControlTask& task,
                                                  unsigned task_flags,
                                                  core::nanoseconds_t deadline) {
    ControlTask::State state;

    if (task_flags & ControlTask::FlagPaused) {
        if (deadline < 0) {
            state = ControlTask::StateCancelling;
        } else if (task_flags & ControlTask::FlagResumed) {
            state = ControlTask::StateProcessing;
        } else {
            state = ControlTask::StateSleeping;
        }
    } else {
        if (deadline < 0) {
            state = ControlTask::StateCancelling;
        } else if (deadline == 0) {
            state = ControlTask::StateProcessing;
        } else {
            state = ControlTask::StateSleeping;
        }
    }

    if (!task.state_.compare_exchange(ControlTask::StateReady, state)) {
        roc_panic("control task queue: unexpected non-ready task in renew: ptr=%p",
                  (void*)&task);
    }

    return state;
}

bool ControlTaskQueue::renew_scheduling_(ControlTask& task,
                                         unsigned task_flags,
                                         core::nanoseconds_t deadline,
                                         core::seqlock_version_t version) {
    // Catch bugs.
    ControlTask::validate_deadline(deadline, version);

    bool is_ready = false;

    if (deadline >= 0) {
        if (task_flags & ControlTask::FlagPaused) {
            // If the task is paused, we either resume it or keep it sleeping.
            // We're not applying new scheduling until task completes.
            if (task_flags & ControlTask::FlagResumed) {
                roc_log(LogTrace, "control task queue: resuming task: ptr=%p",
                        (void*)&task);
                is_ready = true;
            } else {
                roc_log(
                    LogTrace,
                    "control task queue: ignoring renew request for paused task: ptr=%p",
                    (void*)&task);
                is_ready = false;
            }
        } else {
            // Task is not paused, handling re-scheduling request.
            is_ready = reschedule_task_(task, deadline, version);
        }
    } else {
        // Handling cancellation request (no matter if task is paused).
        cancel_task_(task, version);
        is_ready = false;
    }

    return is_ready;
}

bool ControlTaskQueue::reschedule_task_(ControlTask& task,
                                        core::nanoseconds_t deadline,
                                        core::seqlock_version_t version) {
    roc_panic_if_not(deadline >= 0);

    roc_log(LogTrace,
            "control task queue:"
            " rescheduling task: ptr=%p deadline=%lld>%lld version=%llu>%llu",
            (void*)&task, (long long)task.effective_deadline_, (long long)deadline,
            (unsigned long long)task.effective_version_, (unsigned long long)version);

    if (paused_queue_.contains(task)) {
        paused_queue_.remove(task);
    }

    if (sleeping_queue_.contains(task)) {
        remove_sleeping_task_(task);
    }

    task.effective_deadline_ = deadline;
    task.effective_version_ = version;

    const bool is_ready = deadline == 0;

    if (!is_ready) {
        roc_log(LogTrace, "control task queue: moving task to sleeping queue: ptr=%p",
                (void*)&task);

        insert_sleeping_task_(task);
    }

    return is_ready;
}

void ControlTaskQueue::cancel_task_(ControlTask& task, core::seqlock_version_t version) {
    roc_log(LogTrace, "control task queue: cancelling task: ptr=%p version=%llu>%llu",
            (void*)&task, (unsigned long long)task.effective_version_,
            (unsigned long long)version);

    // This should not happend. If the task was already cancelled, its completer was
    // already called and the task may be already destroyed. The upper code in control
    // queue should prevent cancelling a task twice even if the user calls async_cancel()
    // twice. However, the following situation is possible:
    //  - user cancels task
    //  - user re-schedules task
    //  - task is added to ready queue
    //  - user cancels it again before it was fetched from ready queue
    // This is a valid case, because task was re-scheduled before second cancel.
    // We distinguish this situation by checking version. If it changed, the
    // task was probably re-scheduled, and it's not legit to panic.
    roc_panic_if_msg(
        task.effective_deadline_ == -1 && task.effective_version_ == version,
        "control task queue: unexpected already cancelled task in cancel: ptr=%p",
        (void*)&task);

    if (paused_queue_.contains(task)) {
        paused_queue_.remove(task);
    }

    if (sleeping_queue_.contains(task)) {
        remove_sleeping_task_(task);
    }

    const unsigned task_flags = task.flags_ = ControlTask::FlagCancelled;

    task.effective_deadline_ = -1;
    task.effective_version_ = version;

    complete_task_(task, task_flags, ControlTask::StateCancelling);
}

void ControlTaskQueue::reborn_task_(ControlTask& task, ControlTask::State from_state) {
    if (!task.state_.compare_exchange(from_state, ControlTask::StateReady)) {
        // If the task is not in expected state, it means that it was already moved
        // to ready queue from another thread.
        return;
    }

    roc_log(LogTrace, "control task queue: reboring task: ptr=%p", (void*)&task);

    ++ready_queue_size_;

    ready_queue_.push_back(task);
}

void ControlTaskQueue::pause_task_(ControlTask& task, ControlTask::State from_state) {
    roc_log(LogTrace, "control task queue: pausing task: ptr=%p", (void*)&task);

    // Move the task to sleeping state.
    // This may fail if another thread didn't see yet that the task is paused and
    // added it to the ready queue. In this rare case we will fetch the task from
    // the ready queue, see that it's paused, and move to sleeping state again.
    task.state_.compare_exchange(from_state, ControlTask::StateSleeping);

    // Keep track of paused tasks.
    paused_queue_.push_back(task);
}

void ControlTaskQueue::complete_task_(ControlTask& task,
                                      unsigned task_flags,
                                      ControlTask::State from_state) {
    roc_log(LogTrace,
            "control task queue: completing task:"
            " ptr=%p version=%llu is_succeeded=%d is_cancelled=%d has_completer=%d",
            (void*)&task, (unsigned long long)task.effective_version_,
            !!(task_flags & ControlTask::FlagSucceeded),
            !!(task_flags & ControlTask::FlagCancelled), int(task.completer_ != NULL));

    roc_panic_if_msg(task_flags & ControlTask::FlagPaused,
                     "control task queue: unexpected paused task in complete: ptr=%p",
                     (void*)&task);

    IControlTaskCompleter* completer = task.completer_;

    task.state_.compare_exchange(from_state, ControlTask::StateCompleting);

    core::Semaphore* sem = task.sem_.exchange(NULL);

    if (!task.state_.compare_exchange(ControlTask::StateCompleting,
                                      ControlTask::StateCompleted)) {
        roc_log(LogTrace,
                "control task queue: task rescheduled during processing: ptr=%p",
                (void*)&task);

        // Task was re-scheduled while we were processing it.
        // We won't mark it finished and thus won't post the semaphore this time.
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
    if (!task.wait_guard_.compare_exchange(false, true)) {
        roc_panic("control task queue:"
                  " concurrent wait() for the same task not supported: ptr=%p",
                  (void*)&task);
    }

    // Attach a semaphore to the task, if it's not attached yet.
    if (!task.sem_holder_) {
        task.sem_holder_.reset(new (task.sem_holder_) core::Semaphore);
    }
    task.sem_ = task.sem_holder_.get();

    // When the task is in StateCompleting, complete_task_() reads the semaphore
    // from task.sem_. Ensure that we're either before or after this block to avoid race.
    // There are only a few instructions between StateCompleting and
    // StateCompleted, so this spin loop should be very short and rare.
    while (task.state_ == ControlTask::StateCompleting) {
        core::cpu_relax();
    }

    // If the task is not in StateCompleted, it means that it's before
    // StateCompleting (because of the spin loop above), and thus complete_task_
    // will guaranteedly see the semaphore and call post, so we can safely wait on the
    // semaphore.
    //
    // If the task is in StateCompleted, and task.sem_ is NULL, it means that
    // complete_task_ successfully exchanged task.sem_ (which was non-NULL) with NULL, so
    // it will call post, so we can safely wait on the semaphore.
    //
    // Otherwise, i.e. if the task is in StateCompleted and task.sem_ is
    // non-NULL, it means that complete_task_ didn't see the semaphore and so won't call
    // post, so we should not and don't need to wait on it.
    //!
    //! This implementation is so tricky because we're attaching the semaphore only
    //! when wait() is called instead of creating it in the task constructor. This
    //! allows us to avoid an unnecessary syscall for semaphore creation (on platforms
    //! which require such a syscall) for tasks for which wait() is never called or
    //! called only after they actually finish, which is the most common case.
    if (task.state_ != ControlTask::StateCompleted || task.sem_ == NULL) {
        task.sem_holder_->wait();
    }

    task.sem_ = NULL;
    task.wait_guard_ = false;
}

void ControlTaskQueue::execute_task_(ControlTask& task) {
    roc_log(LogTrace, "control task queue: executing task: ptr=%p", (void*)&task);

    roc_panic_if_not(task.effective_deadline_ >= 0);

    roc_panic_if_msg(!task.executor_, "control task queue: task executor is null: ptr=%p",
                     (void*)&task);
    roc_panic_if_msg(!task.func_, "control task queue: task function is null: ptr=%p",
                     (void*)&task);

    // Clear resume flag because we ignore all resume requests issued before execution
    // and should track resume requests issues during or after execution. Also clear
    // success and cancellation flags.
    {
        const unsigned task_flags =
            (task.flags_ &=
             uint8_t(~(ControlTask::FlagSucceeded | ControlTask::FlagCancelled
                       | ControlTask::FlagResumed)));
        // Catch bugs.
        ControlTask::validate_flags(task_flags);
    }

    // Actually execute the task.
    const ControlTaskResult result = task.executor_->execute_task(task, task.func_);

    switch (result) {
    case ControlTaskSuccess:
    case ControlTaskFailure: {
        // Clear all flags, including pause flag, and probably set success flag.
        const unsigned task_flags = task.flags_ =
            (result == ControlTaskSuccess ? ControlTask::FlagSucceeded : 0);

        // Catch bugs.
        ControlTask::validate_flags(task_flags);

        // Check if the task was renewed since it was fetched from the queue.
        // It's important to do this only after clearing the pause flag above, because
        // while pause flag is set, request_renew_() may exit without adding
        // the task to ready queue. It's also important to do this before completing
        // the task because if the task was not renewed, complete_task_() may destroy it.
        core::nanoseconds_t new_deadline = 0;
        core::seqlock_version_t new_version = 0;
        const bool task_renewed =
            task.renewed_deadline_.try_load_v(new_deadline, new_version)
            && new_version != task.effective_version_ && new_deadline >= 0;

        // Notify completer and semaphore that task is finished.
        complete_task_(task, task_flags, ControlTask::StateProcessing);

        // If the task was renewed during pause, the task may be not added to the
        // ready queue. In this case we should do it now.
        if (task_renewed) {
            reborn_task_(task, ControlTask::StateCompleted);
        }
    } break;

    case ControlTaskPause: {
        // Enable pause flag.
        // Since now request_renew_guarded_() won't add task to the ready queue.
        const unsigned task_flags = (task.flags_ |= ControlTask::FlagPaused);

        // Catch bugs.
        ControlTask::validate_flags(task_flags);

        // Move task to sleeping state and add to pause queue.
        pause_task_(task, ControlTask::StateProcessing);

        // Check if the task was cancelled since it was fetched from the queue.
        // It's important to do this only after moving task to sleeping state, because
        // while the task was not in sleeping state, request_renew_() may exit without
        // adding the task to the ready queue.
        core::nanoseconds_t new_deadline = 0;
        const bool task_cancelled =
            task.renewed_deadline_.try_load(new_deadline) && new_deadline < 0;

        // If the task was cancelled during processing, the task may be not added to the
        // ready queue. Usually it's okay because after processing the task completes.
        // But if the task is pausing instead, we should proceed cancellation here.
        if (task_cancelled) {
            reborn_task_(task, ControlTask::StateProcessing);
        }
    } break;

    case ControlTaskContinue: {
        // Disable pause flag, so that the task can be scheduled normally.
        const unsigned task_flags = (task.flags_ &= uint8_t(~ControlTask::FlagPaused));

        // Catch bugs.
        ControlTask::validate_flags(task_flags);

        // The task wants to be executed again, so we just re-add it the ready queue.
        // We don't execute it immediately right here to give other tasks a chance to
        // be executed too and to provent one greedy task blocking the whole queue.
        reborn_task_(task, ControlTask::StateProcessing);
    } break;

    default:
        roc_panic("control task queue: invalid task result: ptr=%p", (void*)&task);
    }
}

bool ControlTaskQueue::process_tasks_() {
    core::Mutex::Lock lock(task_mutex_);

    for (;;) {
        ControlTask* task = fetch_task_();

        if (!task) {
            if (update_wakeup_timer_() == 0) {
                continue;
            }
            return !stop_;
        }

        execute_task_(*task);
    }
}

ControlTask* ControlTaskQueue::fetch_task_() {
    ControlTask* task = NULL;

    // Interleave ready and sleeping tasks to prevent starvation
    // of one of the categories.
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

    return task;
}

ControlTask* ControlTaskQueue::fetch_ready_task_() {
    for (;;) {
        // try_pop_front_exclusive() returns NULL if queue is empty or push_back() is
        // in progress; in the later case ready_queue_size_ is guaranteed to be
        // non-zero and process_tasks_() will call us again soon.
        ControlTask* task = ready_queue_.try_pop_front_exclusive();
        if (!task) {
            roc_log(LogTrace,
                    "control task queue: ready task queue is empty or being pushed");
            return NULL;
        }

        const unsigned task_flags = task->flags_;

        // Catch bugs.
        ControlTask::validate_flags(task_flags);

        core::nanoseconds_t task_deadline = 0;
        core::seqlock_version_t task_version = 0;

        if (!task->renewed_deadline_.try_load_v(task_deadline, task_version)) {
            // Renewed_deadline is being updated concurrently.
            // Re-add task to the queue to try again later.
            roc_log(LogTrace,
                    "control task queue:"
                    " re-adding task to ready queue after first read: ptr=%p",
                    (void*)task);

            ready_queue_.push_back(*task);
            continue;
        }

        // Switch task state based on the renewed deadline.
        const ControlTask::State new_state =
            renew_state_(*task, task_flags, task_deadline);

        // If request_renew_() or request_resume_() was called after we've read deadline
        // and flags, but before we switched the task state, we should re-add the task
        // to ready queue and proceed to next task. This provides a guarantee that if the
        // task was in ready state after making changes to flags or deadline, then event
        // loop thread will certainly see these changes.
        if (task->renewed_deadline_.version() != task_version
            || task->flags_ != task_flags) {
            roc_log(LogTrace,
                    "control task queue:"
                    " re-adding task to ready queue after second read: ptr=%p",
                    (void*)task);

            if (task->state_.compare_exchange(new_state, ControlTask::StateReady)) {
                ready_queue_.push_back(*task);
            } else {
                --ready_queue_size_;
            }
            continue;
        }

        // This will probably destroy the task (if deadline is negative).
        const bool is_ready =
            renew_scheduling_(*task, task_flags, task_deadline, task_version);

        // The task was removed from the queue, we can now handle it.
        // Don't do it before renewing task, to prevent unnecessary attempt to renew
        // it in-place from another thread.
        --ready_queue_size_;

        if (!is_ready) {
            // This task should not be processed, it was added to ready queue
            // just for renewal.
            continue;
        }

        roc_log(LogTrace,
                "control task queue: fetched ready task:"
                " ptr=%p deadline=%llu version=%llu is_paused=%d is_resumed=%d",
                (void*)task, (unsigned long long)task_deadline,
                (unsigned long long)task_version,
                !!(task_flags & ControlTask::FlagPaused),
                !!(task_flags & ControlTask::FlagResumed));

        return task;
    }
}

ControlTask* ControlTaskQueue::fetch_sleeping_task_() {
    ControlTask* task = sleeping_queue_.front();
    if (!task) {
        return NULL;
    }

    if (task->effective_deadline_ > core::timestamp(core::ClockMonotonic)) {
        return NULL;
    }

    remove_sleeping_task_(*task);

    if (!task->state_.compare_exchange(ControlTask::StateSleeping,
                                       ControlTask::StateProcessing)) {
        return NULL;
    }

    roc_log(LogTrace, "control task queue: fetched sleeping task: ptr=%p deadline=%lld",
            (void*)task, (long long)task->effective_deadline_);

    return task;
}

void ControlTaskQueue::insert_sleeping_task_(ControlTask& task) {
    roc_panic_if_not(task.effective_deadline_ > 0);

    ControlTask* pos = sleeping_queue_.front();

    for (; pos; pos = sleeping_queue_.nextof(*pos)) {
        if (pos->effective_deadline_ > task.effective_deadline_) {
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
    roc_panic_if_not(task.effective_deadline_ > 0);

    sleeping_queue_.remove(task);
}

core::nanoseconds_t ControlTaskQueue::update_wakeup_timer_() {
    core::nanoseconds_t deadline = 0;

    // Sleep only if there are no tasks in ready queue.
    if (ready_queue_size_ == 0) {
        if (ControlTask* task = sleeping_queue_.front()) {
            deadline = task->effective_deadline_;
        } else {
            deadline = -1;
        }
    }

    roc_log(LogTrace, "control task queue: updating wakeup deadline: deadline=%lld",
            (long long)deadline);

    wakeup_timer_.try_set_deadline(deadline);

    // We should check whether new tasks were added while we were updating the timer.
    // In this case, try_set_deadline(0) in request_renew_() was probably failed, and
    // we should call it by ourselves to wake up the event loop thread.
    if (deadline != 0 && ready_queue_size_ != 0) {
        deadline = 0;
        wakeup_timer_.try_set_deadline(0);
    }

    return deadline;
}

} // namespace ctl
} // namespace roc
