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
#include "roc_core/thread.h"
#include "roc_core/time.h"
#include "roc_core/timer.h"
#include "roc_ctl/control_task.h"
#include "roc_ctl/control_task_executor.h"
#include "roc_ctl/icontrol_task_completer.h"

namespace roc {
namespace ctl {

//! Control task queue.
//!
//! This class implements a thread-safe task queue, allowing lock-free scheduling
//! of tasks for immediate or delayed execution on the background thread, as well
//! as lock-free task cancellation and re-scheduling (changing deadline).
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
//! The implementation uses two queues internally:
//!
//!  - ready_queue_ - a lock-free queue of tasks of three kinds:
//!    - tasks to be executed as soon as possible (i.e. with zero deadline)
//!    - tasks to be re-scheduled with another deadline (renewed_deadline_ > 0)
//!    - tasks to be cancelled                          (renewed_deadline_ < 0)
//!
//!  - sleeping_queue_ - a sorted queue of tasks with non-zero deadline, scheduled for
//!    execution in future; the task at the head has the smallest (nearest) deadline;
//!
//! task_mutex_ should be acquired to process tasks and/or to access sleeping_queue_.
//!
//! wakeup_timer_ (core::Timer) is used to set or wait for the next wakeup time of the
//! background thread. This time is set to zero when ready_queue_ is non-empty, otherwise
//! it is set to the deadline of the first task in sleeping_queue_ if it's non-empty, and
//! otherwise is set to infinity (-1). The timer allows to update the deadline
//! concurrently from any thread.
//!
//! When the task is scheduled, re-scheduled, or cancelled, there are two ways to
//! complete the operation:
//!
//!  - If the event loop thread is sleeping and the task_mutex_ is free, we can acquire
//!    the mutex and complete the operation in-place by manipulating sleeping_queue_
//!    under the mutex.
//!
//!  - Otherwise, we push the task to ready_queue_ (which has lock-free push), set
//!    the timer wakeup time to zero (to ensure that the background thread wont go to
//!    sleep), and return, leaving the completion of the operarion to the background
//!    thread. The background will fetch the task from ready_queue_ soon and complete
//!    the operation by manipulating the sleeping_queue_.
//!
//! The current task state is defined by its atomic field "state_". Various task queue
//! operations move task from one state to another. The move is always performed using
//! atomic CAS or exchange to handle concurrent lock-free updates correctly.
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
//!    async_cancel():
//!      StateSleeping -> StateReady
//!        -> StateCancelling -> StateCompleting -> StateCompleted
//! @endcode
//!
//! The meaning of the states is the following:
//!  - StateReady: task is added to the ready queue for execution or renewal,
//!                or probably is currently being renewed in-place
//!  - StateSleeping: task renewal is complete and the task was put into the sleeping
//!                   queue because it was put to ready queue for re-scheduling
//!  - StateCancelling: task renewal is complete and the task is being cancelled
//!                     because it was put to ready queue for cancallation
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
    bool valid() const;

    //! Enqueue a task for asynchronous execution as soon as possible.
    //!
    //! If the task is completed, it becomes pending.
    //! If the task is already pending, its scheduled time is changed.
    //! If the task is executing currently, it will be re-scheduled after it completes.
    //!
    //! The task should not be destroyed until the completer is called, if it's present.
    //! The task will be executed asynchronously as soon as possible.
    //!
    //! The @p executor is used to execute the task method.
    //! The @p completer is invoked on event loop thread after the task completes.
    //! Completer should be non-blocking.
    void schedule(ControlTask& task,
                  IControlTaskExecutor& executor,
                  IControlTaskCompleter* completer);

    //! Enqueue a task for asynchronous execution at given point of time.
    //!
    //! If the task is completed, it becomes pending.
    //! If the task is already pending, its scheduled time is changed.
    //! If the task is executing currently, it will be re-scheduled after it completes.
    //!
    //! The task should not be destroyed until the completer is called, if it's present.
    //! The task will be executed asynchronously after deadline expires.
    //!
    //! @p deadline should be in the same domain as core::timestamp().
    //! It can't be negative. Zero deadline means "execute as soon as possible".
    //!
    //! The @p executor is used to execute the task method.
    //! The @p completer is invoked on event loop thread after the task completes.
    //! Completer should be non-blocking.
    void schedule_at(ControlTask& task,
                     core::nanoseconds_t deadline,
                     IControlTaskExecutor& executor,
                     IControlTaskCompleter* completer);

    //! Cancel scheduled task execution.
    //!
    //! If the task is executing currently or is already completed, do nothing.
    //! If the task is pending, cancel scheduled execution.
    //! If the task has completer, and it was not called yet, and the
    //! task was cancelled, the completer will be invoked. In other words, no
    //! matter whether the task was executed or cancelled, the completer is guaranteed
    //! to be invoked once.
    void async_cancel(ControlTask& task);

    //! Wait until the task is completed.
    //!
    //! Blocks until the task is executed or cancelled.
    //! Does NOT wait until the task completer is called.
    //!
    //! Can not be called concurrently for the same task.
    //! Can not be called from the task completion handler.
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

    void start_thread_();
    void stop_thread_();

    bool process_tasks_();

    void setup_task_(ControlTask& task,
                     IControlTaskExecutor& executor,
                     IControlTaskCompleter* completer);
    void renew_task_(ControlTask& task, core::nanoseconds_t deadline);
    void enqueue_renewed_task_(ControlTask& task, core::nanoseconds_t deadline);

    bool try_renew_deadline_inplace_(ControlTask& task, core::nanoseconds_t deadline);

    ControlTask::State apply_renewed_state_(ControlTask& task,
                                            core::nanoseconds_t deadline);
    void apply_renewed_deadline_(ControlTask& task, core::nanoseconds_t deadline);

    void reschedule_task_(ControlTask& task, core::nanoseconds_t deadline);
    void cancel_task_(ControlTask& task);

    void execute_task_(ControlTask& task);
    void complete_task_(ControlTask& task, ControlTask::State from_state);
    void wait_task_(ControlTask& task);

    ControlTask* fetch_ready_task_();
    ControlTask* fetch_ready_or_renewed_task_(core::nanoseconds_t& renewed_deadline);
    ControlTask* fetch_sleeping_task_();

    void insert_sleeping_task_(ControlTask& task);
    void remove_sleeping_task_(ControlTask& task);

    core::nanoseconds_t update_wakeup_timer_();

    bool started_;
    core::Atomic<int> stop_;
    bool fetch_ready_;

    core::Atomic<int> ready_queue_size_;
    core::MpscQueue<ControlTask, core::NoOwnership> ready_queue_;
    core::List<ControlTask, core::NoOwnership> sleeping_queue_;

    core::Timer wakeup_timer_;
    core::Mutex task_mutex_;
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_CONTROL_TASK_QUEUE_H_
