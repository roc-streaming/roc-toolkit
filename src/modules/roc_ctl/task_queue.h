/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_ctl/task_queue.h
//! @brief Task queue thread.

#ifndef ROC_CTL_TASK_QUEUE_H_
#define ROC_CTL_TASK_QUEUE_H_

#include "roc_core/atomic.h"
#include "roc_core/cond.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/mutex.h"
#include "roc_core/thread.h"
#include "roc_core/time.h"
#include "roc_core/timer.h"

namespace roc {
namespace ctl {

//! Task queue thread.
class TaskQueue : private core::Thread {
public:
    class ICompletionHandler;

    //! Base task class.
    //! The user is responsible for allocating and deallocating the task.
    class Task : public core::ListNode {
    public:
        ~Task();

        //! Check if the task was cancelled and was not executed.
        bool cancelled() const;

        //! Check if the task was executed and succeeded.
        bool success() const;

    protected:
        Task();

    private:
        friend class TaskQueue;

        void set_scheduling_params_(core::nanoseconds_t delay,
                                    ICompletionHandler* handler);

        void set_deadline_(core::nanoseconds_t delay);

        void reset_state_(bool pending);

        core::nanoseconds_t deadline_;

        // result_ should be set before setting pending_ to false
        core::Atomic result_;
        core::Atomic pending_;

        bool request_cancel_;

        ICompletionHandler* handler_;
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

    //! Enqueue a task for asynchronous execution and return.
    //! The task should not be destroyed until it's finishes and the handler is called.
    //! The @p handler is invoked on event loop thread after the task completes.
    //! It should not block the caller.
    void schedule(Task& task, ICompletionHandler* handler);

    //! Enqueue a task for asynchronous execution after given delay, and return.
    //! The task will be executed asynchronously after the @p delay expires.
    //! The task should not be destroyed until it's finishes and the handler is called.
    //! The @p handler is invoked on event loop thread after the task completes.
    //! It should not block the caller.
    void
    schedule_after(Task& task, core::nanoseconds_t delay, ICompletionHandler* handler);

    //! Cancel task if it's already scheduled and re-schedule it with a new deadline.
    //! Works like a combination of cancel_and_wait() and schedule_after(), but
    //! asynchronously without blocking the caller.
    //! The previois invocation of the completion handler may be cancelled, but
    //! it's not guaranteed.
    //! If this method is called, the task should not be destroyed until it's
    //! completion handler is invoked for the new schedule.
    void reschedule_after(Task& task, core::nanoseconds_t delay);

    //! Enqueue a task for asynchronous execution and wait for its completion.
    //! The task should not be destroyed until the method returns.
    //! Should not be called from ICompletionHandler.
    //! @returns
    //!  true if the task succeeded or false if it failed.
    bool schedule_and_wait(Task& task);

    //! Asynchronously cancel scheduled task, if it was not executed yet.
    //! If the task was not executed yet, it will be either cancelled or executed,
    //! depending on whether its deadline is already expired.
    //! If ICompletionHandler is present and was not called yet, it will be called
    //! soon, no matter whether the task was cancelled or executed.
    void async_cancel(Task& task);

    //! Asynchronously cancel scheduled task and wait until it cancelled or finished.
    //! If the task was not executed yet, it will be either cancelled or executed,
    //! depending on whether its deadline is already expired.
    //! If ICompletionHandler is present and was not called yet, it will be called
    //! soon, no matter whether the task was cancelled or executed.
    void cancel_and_wait(Task& task);

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
    virtual void run();

    Task* begin_task_processing_();
    void process_task_(Task&);
    void end_task_processing_();

    void schedule_task_(Task&);
    void reschedule_task_(Task&, core::nanoseconds_t delay);
    void cancel_task_(Task&);

    void add_to_pending_(Task&);
    void remove_from_pending_(Task&);

    void update_next_deadline_();

    bool started_;
    bool stop_;
    bool request_reschedule_;

    core::List<Task, core::NoOwnership> pending_tasks_;
    Task* first_task_with_deadline_;
    Task* currently_processing_task_;

    core::Mutex mutex_;
    core::Timer wakeup_timer_;
    core::Cond finished_cond_;
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_TASK_QUEUE_H_
