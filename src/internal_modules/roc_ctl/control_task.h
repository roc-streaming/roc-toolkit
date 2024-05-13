/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_ctl/control_task.h
//! @brief Control task.

#ifndef ROC_CTL_CONTROL_TASK_H_
#define ROC_CTL_CONTROL_TASK_H_

#include "roc_core/atomic.h"
#include "roc_core/list_node.h"
#include "roc_core/mpsc_queue_node.h"
#include "roc_core/mutex.h"
#include "roc_core/optional.h"
#include "roc_core/semaphore.h"
#include "roc_core/seqlock.h"
#include "roc_core/time.h"

namespace roc {
namespace ctl {

class ControlTaskQueue;
class ControlTask;

class IControlTaskExecutor;
class IControlTaskCompleter;

//! Control task execution result.
enum ControlTaskResult {
    //! Task completed with success.
    ControlTaskSuccess,

    //! Task completed with failure.
    ControlTaskFailure,

    //! Task wants to be re-executed again as soon as possible.
    ControlTaskContinue,

    //! Task wants to be paused until is explicitly resumed.
    ControlTaskPause
};

//! Control task implementation function.
//! Holds a pointer to method of a class derived from IControlTaskExecutor.
typedef ControlTaskResult (IControlTaskExecutor::*ControlTaskFunc)(ControlTask&);

//! Base class for control tasks.
class ControlTask : public core::MpscQueueNode<>, public core::ListNode<> {
public:
    ~ControlTask();

    //! True if the task succeeded, failed, or cancelled.
    bool completed() const;

    //! True if the task succeeded.
    bool succeeded() const;

    //! True if the task cancelled.
    bool cancelled() const;

protected:
    //! Initialize task.
    //! @tparam E is a class derived from IControlTaskExecutor.
    //! @p task_func is a method of E which implements the task.
    template <class E>
    ControlTask(ControlTaskResult (E::*task_func)(ControlTask&))
        : state_(StateCompleted)
        , flags_(0)
        , renew_guard_(false)
        , wait_guard_(false)
        , renewed_deadline_(0)
        , effective_deadline_(0)
        , effective_version_(0)
        , func_(reinterpret_cast<ControlTaskFunc>(task_func))
        , executor_(NULL)
        , completer_(NULL)
        , sem_(NULL) {
        // ensure that E implements IControlTaskExecutor
        (void)static_cast<IControlTaskExecutor*>((E*)NULL);
    }

private:
    friend class ControlTaskQueue;

    enum State {
        // task is in ready queue or being fetched from it; after it's
        // fetched, it will be processed, cancelled, or rescheduled
        StateReady,

        // task is in sleeping queue, waiting for its deadline
        StateSleeping,

        // task cancellation is initiated
        StateCancelling,

        // task is being processed, it's executing or will be executed soon
        StateProcessing,

        // task completion is initiated
        StateCompleting,

        // task is completed and is not used
        StateCompleted
    };

    enum Flag {
        // last execution succeeded
        FlagSucceeded = (1 << 0),

        // last execution paused
        FlagPaused = (1 << 2),

        // task resuming was requested
        FlagResumed = (1 << 3),

        // task was cancelled
        FlagCancelled = (1 << 4),

        // task destructor was called
        // seeing this flag indicates use-after-free bug
        FlagDestroyed = (1 << 5)
    };

    // validate task properties
    static void validate_flags(unsigned task_flags);
    static void validate_deadline(core::nanoseconds_t deadline,
                                  core::seqlock_version_t version);

    // scheduling state of the task
    core::Atomic<uint32_t> state_;

    // additional details about current state
    core::Atomic<uint32_t> flags_;

    // guard to cut off concurrent task renewals (only one succeeds)
    core::Atomic<uint32_t> renew_guard_;

    // guard to cut off concurrent task waits (only one allowed)
    core::Atomic<uint32_t> wait_guard_;

    // new task deadline that is probably not yet taken into account
    core::Seqlock<core::nanoseconds_t> renewed_deadline_;

    // currently active task deadline, defines when to execute task:
    // > 0: absolute time of execution
    // = 0: execute as soon as possible
    // < 0: cancel task
    core::nanoseconds_t effective_deadline_;

    // version of currently active task deadline
    core::seqlock_version_t effective_version_;

    // function to be executed
    ControlTaskFunc func_;

    // object that executes task function
    IControlTaskExecutor* executor_;

    // object that is notified when the task completes or cancels
    IControlTaskCompleter* completer_;

    // semaphore to wait for task completion
    core::Atomic<core::Semaphore*> sem_;
    core::Optional<core::Semaphore> sem_holder_;
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_CONTROL_TASK_H_
