/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_ctl/task_queue.h
//! @brief Asynchronous task queue.

#ifndef ROC_CTL_TASK_QUEUE_H_
#define ROC_CTL_TASK_QUEUE_H_

#include "roc_core/atomic.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/mpsc_queue.h"
#include "roc_core/mpsc_queue_node.h"
#include "roc_core/mutex.h"
#include "roc_core/optional.h"
#include "roc_core/semaphore.h"
#include "roc_core/seqlock.h"
#include "roc_core/thread.h"
#include "roc_core/time.h"
#include "roc_core/timer.h"

namespace roc {
namespace ctl {

//! Asynchronous task queue.
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
//! This design was considered acceptable because the actual task queue users are more
//! sensitive to delays than the tasks they schedule. The task queue is used by network
//! and pipeline threads, which should never block and use the task queue to schedule
//! low-priority delayed work.
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
//! Here are some typical flows of the task states:
//! @code
//!    schedule():
//!      StateFinished -> StateInitializing -> StateReady
//!        -> StateProcessing -> StateFinishing -> StateFinished
//!
//!    schedule_at():
//!      StateFinished -> StateInitializing -> StateReady
//!        -> StateSleeping
//!        -> StateProcessing -> StateFinishing -> StateFinished
//!
//!    reschedule_at():
//!      [any state] -> StateReady
//!        -> StateSleeping
//!        -> StateProcessing -> StateFinishing -> StateFinished
//!
//!    async_cancel():
//!      StateSleeping -> StateReady
//!        -> StateCancelling -> StateFinishing -> StateFinished
//! @endcode
//!
//! The meaning of the states is the following:
//!  - StateInitializing: task is being initialized by schedule() or schedule_at()
//!  - StateReady: task is added to the ready queue for execution or renewal,
//!                or probably is currently being renewed in-place
//!  - StateSleeping: task renewal is complete and the task was put into the sleeping
//!                   queue because it was put to ready queue for re-scheduling
//!  - StateCancelling: task renewal is complete and the task is being cancelled
//!                     because it was put to ready queue for cancallation
//!  - StateProcessing: task is being processed after fetching it either from ready
//!                     queue (if it was put there for execution) or sleeping queue
//!  - StateFinishing: task processing is complete and the task is being finished
//!  - StateFinished: task is finished and is not used anywhere; it may be safely
//!                   destroyed; this is also the initial task state
class TaskQueue : private core::Thread {
public:
    class ICompletionHandler;

    //! Base task class.
    //! The user is responsible for allocating and deallocating the task.
    class Task : public core::MpscQueueNode, public core::ListNode {
    public:
        ~Task();

        //! True if the task is not finished yet.
        bool pending() const;

        //! True if the latest execution succeeded.
        bool success() const;

        //! True if the latest execution was cancelled.
        bool cancelled() const;

    protected:
        Task();

    private:
        friend class TaskQueue;

        core::Atomic<int> state_;
        core::Atomic<int> result_;

        core::nanoseconds_t deadline_;
        core::Seqlock<core::nanoseconds_t> renewed_deadline_;

        core::Atomic<int> renew_in_progress_;
        core::Atomic<int> wait_in_progress_;

        ICompletionHandler* handler_;

        core::Optional<core::Semaphore> sem_obj_;
        core::Atomic<core::Semaphore*> sem_;
    };

    //! Task completion handler.
    class ICompletionHandler {
    public:
        virtual ~ICompletionHandler();

        //! Called when a task is finished.
        virtual void control_task_finished(Task&) = 0;
    };

    //! Initialize.
    //! @remarks
    //!  Starts background thread.
    TaskQueue();

    //! Destroy.
    //! @remarks
    //!  stop() should be called before destructor.
    virtual ~TaskQueue();

    //! Check if the object was successfully constructed.
    bool valid() const;

    //! Enqueue a task for asynchronous execution in the nearest future.
    //! Can be called only after the task is finished.
    //! The task should not be destroyed until the handler is called, if it's set.
    //! The @p handler is invoked on event loop thread after the task finishes.
    //! The handler should not block the caller.
    void schedule(Task& task, ICompletionHandler* handler);

    //! Enqueue a task for asynchronous execution at given point of time.
    //! Can be called only after the task is finished.
    //! The task will be executed asynchronously after deadline expires.
    //! @p deadline should be in the same domain as core::timestamp().
    //! It can't be negative. Zero deadline means "execute as soon as possible".
    //! The task should not be destroyed until the handler is called, if it's set.
    //! The @p handler is invoked on event loop thread after the task finishes.
    //! The handler should not block the caller.
    void
    schedule_at(Task& task, core::nanoseconds_t deadline, ICompletionHandler* handler);

    //! Re-schedule a task with another deadline.
    //! If the task is finished, it becomes pending again.
    //! If the task is already pending, its scheduling time is changed.
    //! If the task is executing currently, it will be re-scheduled after it finishes.
    //! @p deadline should be in the same domain as core::timestamp().
    //! It can't be negative. Zero deadline means "execute as soon as possible".
    //! After this call, the task should not be destroyed until the task finishes.
    //! If the task has completion handler, it will be invoked again.
    //! It's guaranteed that the task will be executed at least once after this call
    //! (one possible execution for the previous schedule if the deadline was just
    //! expired, and one guaranteed execution for the new schedule).
    void reschedule_at(Task& task, core::nanoseconds_t deadline);

    //! Cancel scheduled task execution.
    //! If the task is executing currently or is already finished, do nothing.
    //! If the task is pending, cancel scheduled execution.
    //! If the task has completion handler, and it was not called yet, and the
    //! task was cancelled, the handler will be invoked. In other words, no
    //! matter whether the task was executed or cancelled, the handler is guaranteed
    //! to be invoked once.
    void async_cancel(Task& task);

    //! Wait until the task is finished.
    //! Blocks until the task is executed or cancelled.
    //! Can not be called concurrently for the same task.
    //! Can not be called from the task completion handler.
    //! Does NOT wait until the task completion handler is called.
    //! If this method is called, the task should not be destroyed until this method
    //! returns (as well as until the completion handler is called, if set).
    void wait(Task& task);

protected:
    //! Task execution result.
    enum TaskResult { TaskSucceeded, TaskFailed, TaskCancelled };

    //! Task processing implementation.
    virtual TaskResult process_task_imp(Task&) = 0;

    //! Stop thread and wait until it terminates.
    //! All tasks should be finished before calling stop().
    //! stop() should be called before calling destructor.
    void stop_and_wait();

private:
    // Task states.
    enum TaskState {
        StateInitializing,
        StateReady,
        StateSleeping,
        StateCancelling,
        StateProcessing,
        StateFinishing,
        StateFinished
    };

    virtual void run();

    bool process_tasks_();

    void initialize_task_(Task& task, ICompletionHandler* handler);
    void renew_task_(Task& task, core::nanoseconds_t deadline);
    void enqueue_renewed_task_(Task& task, core::nanoseconds_t deadline);

    bool try_renew_deadline_inplace_(Task& task, core::nanoseconds_t deadline);

    TaskState apply_renewed_state_(Task& task, core::nanoseconds_t deadline);
    void apply_renewed_deadline_(Task& task, core::nanoseconds_t deadline);

    void reschedule_task_(Task& task, core::nanoseconds_t deadline);
    void cancel_task_(Task& task);

    void process_task_(Task& task);
    void finish_task_(Task& task, TaskState from_state);
    void wait_task_(Task& task);

    Task* fetch_ready_task_();
    Task* fetch_ready_or_renewed_task_(core::nanoseconds_t& renewed_deadline);
    Task* fetch_sleeping_task_();

    void insert_sleeping_task_(Task& task);
    void remove_sleeping_task_(Task& task);

    core::nanoseconds_t update_wakeup_timer_();

    bool started_;
    core::Atomic<int> stop_;

    core::Atomic<int> ready_queue_size_;
    core::MpscQueue<Task, core::NoOwnership> ready_queue_;
    core::List<Task, core::NoOwnership> sleeping_queue_;

    core::Timer wakeup_timer_;
    core::Mutex task_mutex_;
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_TASK_QUEUE_H_
