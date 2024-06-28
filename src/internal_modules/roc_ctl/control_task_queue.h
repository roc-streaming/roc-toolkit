/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_ctl/control_task_queue.h
//! @brief Control task queue.

#ifndef ROC_CTL_CONTROL_TASK_QUEUE_H_
#define ROC_CTL_CONTROL_TASK_QUEUE_H_

#include "roc_core/atomic.h"
#include "roc_core/list.h"
#include "roc_core/mpsc_queue.h"
#include "roc_core/mutex.h"
#include "roc_core/pairing_heap.h"
#include "roc_core/thread.h"
#include "roc_core/time.h"
#include "roc_core/timer.h"
#include "roc_ctl/control_task.h"
#include "roc_ctl/control_task_executor.h"
#include "roc_ctl/icontrol_task_completer.h"
#include "roc_status/status_code.h"

namespace roc {
namespace ctl {

//! Control task queue.
//!
//! This class implements a thread-safe task queue, allowing lock-free scheduling
//! of tasks for immediate or delayed execution on the background thread, as well
//! as lock-free task cancellation and re-scheduling (changing deadline).
//!
//! It also supports tasks to be paused and resumed. Task resuming is lock-free too.
//!
//! Note that those operations are lock-free only if core::Timer::try_set_deadline()
//! is so, which however is true on modern platforms.
//!
//! In the current implementation, priority is given to fast scheduling and cancellation
//! over the strict observance of the scheduling deadlines. In other words, during
//! contention or peak load, scheduling and cancellation will be always fast, but task
//! execution may be delayed.
//!
//! This design was considered acceptable because the actual users of control task queue
//! are more sensitive to delays than the tasks they schedule. The task queue is used by
//! network and pipeline threads, which should never block and use the task queue to
//! schedule low-priority delayed work.
//!
//! The implementation uses three queues internally:
//!
//!  - ready_queue_ - a lock-free queue of tasks of three kinds:
//!    - tasks to be resumed after pause (flags_ & FlagResumed != 0)
//!    - tasks to be executed as soon as possible (renewed_deadline_ == 0)
//!    - tasks to be re-scheduled with another deadline (renewed_deadline_ > 0)
//!    - tasks to be canceled                           (renewed_deadline_ < 0)
//!
//!  - sleeping_queue_ - a sorted queue of tasks with non-zero deadline, scheduled for
//!    execution in future; the task at the head has the smallest (nearest) deadline;
//!
//!  - pause_queue_ - an unsorted queue to keep track of all currently paused tasks.
//!
//! task_mutex_ should be acquired to process tasks and/or to access sleeping_queue_
//! and pause_queue_, as well as non-atomic task fields.
//!
//! wakeup_timer_ (core::Timer) is used to set or wait for the next wakeup time of the
//! background thread. This time is set to zero when ready_queue_ is non-empty, otherwise
//! it is set to the deadline of the first task in sleeping_queue_ if it's non-empty, and
//! otherwise is set to infinity (-1). The timer allows to update the deadline
//! concurrently from any thread.
//!
//! When the task is scheduled, re-scheduled, or canceled, there are two ways to
//! complete the operation:
//!
//!  - If the event loop thread is sleeping and the task_mutex_ is free, we can acquire
//!    the mutex and complete the operation in-place by manipulating sleeping_queue_
//!    under the mutex, without bothering event loop thread. This can be done only if
//!    we're changing task scheduling and not going to execute it right now.
//!
//!  - Otherwise, we push the task to ready_queue_ (which has lock-free push), set
//!    the timer wakeup time to zero (to ensure that the event loop thread wont go to
//!    sleep), and return, leaving the completion of the operarion to the event loop
//!    thread. The event loop thread will fetch the task from ready_queue_ soon and
//!    complete the operation by manipulating the sleeping_queue_.
//!
//! The current task state is defined by its atomic field "state_". Various task queue
//! operations move task from one state to another. The move is always performed using
//! atomic CAS or exchange to handle concurrent lock-free updates correctly.
//!
//! There is also "flags_" field that provides additional information about task that is
//! preserved across transitions between states; for example that task is being resumed.
//!
//! Here are some example flows of the task states:
//! @code
//!    schedule():
//!      StateCompleted -> StateReady
//!        -> StateProcessing -> StateCompleting -> StateCompleted
//!
//!    schedule_at():
//!      StateCompleted -> StateReady
//!        -> StateSleeping
//!        -> StateProcessing -> StateCompleting -> StateCompleted
//!
//!    resume():
//!      StateSleeping -> StateReady
//!        -> StateProcessing -> StateCompleting -> StateCompleted
//!
//!    async_cancel():
//!      StateSleeping -> StateReady
//!        -> StateCancelling -> StateCompleting -> StateCompleted
//! @endcode
//!
//! The meaning of the states is the following:
//!  - StateReady: task is added to the ready queue for execution or renewal,
//!                or probably is currently being renewed in-place
//!  - StateSleeping: task renewal is complete and the task was put into the sleeping
//!                   queue to wait its deadline, or to paused queue to wait resume
//!  - StateCancelling: task renewal is complete and the task is being canceled
//!                     because it was put to ready queue for cancellation
//!  - StateProcessing: task is being processed after fetching it either from ready
//!                     queue (if it was put there for execution) or sleeping queue
//!  - StateCompleting: task processing is complete and the task is being completed
//!  - StateCompleted: task is completed and is not used anywhere; it may be safely
//!                   destroyed or reused; this is also the initial task state
class ControlTaskQueue : private core::Thread {
public:
    //! Initialize.
    //! @remarks
    //!  Starts background thread.
    ControlTaskQueue();

    //! Destroy.
    //! @remarks
    //!  stop_and_wait() should be called before destructor.
    virtual ~ControlTaskQueue();

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Enqueue a task for asynchronous execution as soon as possible.
    //!
    //! This is like schedule_at(), but the deadline is "as soon as possible".
    void schedule(ControlTask& task,
                  IControlTaskExecutor& executor,
                  IControlTaskCompleter* completer);

    //! Enqueue a task for asynchronous execution at given point of time.
    //!
    //! - If the task is already completed, it's scheduled with given deadline.
    //! - If the task is sleeping and waiting for deadline, it's deadline is updated.
    //! - If the task is in processing, completion or cancellation phase, it's scheduled
    //!   to be executed again after completion or cancellation finishes.
    //! - If the task is paused, re-scheduling is postponed until task resumes.
    //!
    //! @p deadline should be in the same domain as core::timestamp().
    //! It can't be negative. Zero deadline means "execute as soon as possible".
    //!
    //! The @p executor is used to invoke the task function. It allows to implement
    //! tasks in different classes. If a class T wants to implement tasks, it should
    //! inherit ControlTaskExecutor<T>.
    //!
    //! If @p completer is present, the task should not be destroyed until completer is
    //! invoked. The completer is invoked on event loop thread after once and only once,
    //! after the task completes or is canceled. Completer should never block.
    //!
    //! The event loop thread assumes that the task may be destroyed right after it is
    //! completed and it's completer is called (if it's present), and don't touch task
    //! after this, unless the user explicitly reschedules the task.
    void schedule_at(ControlTask& task,
                     core::nanoseconds_t deadline,
                     IControlTaskExecutor& executor,
                     IControlTaskCompleter* completer);

    //! Resume task if it's paused.
    //!
    //! - If the task is paused, schedule it for execution.
    //! - If the task is being processed right now (i.e. it's executing or will be
    //!   executing very soon), then postpone decision until task execution ends. After
    //!   the task execution, if the task asked to pause, then immediately resume it.
    //! - Otherwise, do nothing.
    //!
    //! If resume is called one or multiple times before task execution, those calls
    //! are ignored. Only calls made during or after task execution are honored, and
    //! only if the task execution leaved task in paused state.
    //!
    //! Subsequent resume calls between task executions are collapsed into one; even if
    //! resume was called multiple after task paused and before it's executed again,
    //! next pause will need a new resume call.
    void resume(ControlTask& task);

    //! Try to cancel scheduled task execution, if it's not executed yet.
    //!
    //! - If the task is already completed or is being completed or canceled, do nothing.
    //! - If the task is sleeping or paused, cancel task execution.
    //! - If the task is being processed right now (i.e. it's executing or will be
    //!   executing very soon), then postpone decision until task execution ends. After
    //!   the task execution, if the task asked to pause or continue, then cancellation
    //!   request is fulfilled and the task is canceled; otherwise cancellation request
    //!   is ignored and the task is completed normally.
    //!
    //! When the task is being canceled instead of completed, if it has completer, the
    //! completer is invoked.
    void async_cancel(ControlTask& task);

    //! Wait until the task is completed.
    //!
    //! Blocks until the task is completed or canceled.
    //! Does NOT wait until the task completer is called.
    //!
    //! Can not be called concurrently for the same task (will cause crash).
    //! Can not be called from the task completion handler (will cause deadlock).
    //!
    //! If this method is called, the task should not be destroyed until this method
    //! returns (as well as until the completer is invoked, if it's present).
    void wait(ControlTask& task);

    //! Stop thread and wait until it terminates.
    //!
    //! All tasks should be completed before calling stop_and_wait().
    //! stop_and_wait() should be called before calling destructor.
    void stop_and_wait();

private:
    virtual void run();

    bool start_thread_();
    void stop_thread_();

    void setup_task_(ControlTask& task,
                     IControlTaskExecutor& executor,
                     IControlTaskCompleter* completer);

    void request_resume_(ControlTask& task);
    void request_renew_(ControlTask& task, core::nanoseconds_t deadline);
    void request_renew_guarded_(ControlTask& task, core::nanoseconds_t deadline);

    bool try_renew_inplace_(ControlTask& task,
                            core::nanoseconds_t deadline,
                            core::seqlock_version_t version);

    ControlTask::State
    renew_state_(ControlTask& task, unsigned task_flags, core::nanoseconds_t deadline);
    bool renew_scheduling_(ControlTask& task,
                           unsigned task_flags,
                           core::nanoseconds_t deadline,
                           core::seqlock_version_t version);

    bool reschedule_task_(ControlTask& task,
                          core::nanoseconds_t deadline,
                          core::seqlock_version_t version);
    void cancel_task_(ControlTask& task, core::seqlock_version_t version);

    void reborn_task_(ControlTask& task, ControlTask::State from_state);
    void pause_task_(ControlTask& task, ControlTask::State from_state);
    void
    complete_task_(ControlTask& task, unsigned task_flags, ControlTask::State from_state);
    void wait_task_(ControlTask& task);

    void execute_task_(ControlTask& task);

    bool process_tasks_();

    ControlTask* fetch_task_();
    ControlTask* fetch_ready_task_();
    ControlTask* fetch_sleeping_task_();

    void insert_sleeping_task_(ControlTask& task);
    void remove_sleeping_task_(ControlTask& task);

    core::nanoseconds_t update_wakeup_timer_();

    bool started_;
    core::Atomic<int> stop_;
    bool fetch_ready_;

    core::Atomic<int> ready_queue_size_;
    core::MpscQueue<ControlTask, core::NoOwnership> ready_queue_;
    core::PairingHeap<ControlTask, core::NoOwnership> sleeping_queue_;
    core::List<ControlTask, core::NoOwnership> paused_queue_;

    core::Timer wakeup_timer_;
    core::Mutex task_mutex_;

    status::StatusCode init_status_;
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_CONTROL_TASK_QUEUE_H_
