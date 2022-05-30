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
    //! Task completed successfully.
    ControlTaskSucceeded,

    //! Task failed.
    ControlTaskFailed,

    //! Task was cancelled before completion.
    ControlTaskCancelled
};

//! Control task implementation function.
//! Holds a pointer to method of a class derived from IControlTaskExecutor.
typedef ControlTaskResult (IControlTaskExecutor::*ControlTaskFunc)(ControlTask&);

//! Base class for control tasks.
class ControlTask : public core::MpscQueueNode, public core::ListNode {
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
        , result_(ControlTaskFailed)
        , deadline_(0)
        , renewed_deadline_(0)
        , renew_in_progress_(false)
        , wait_in_progress_(false)
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
        StateReady,
        StateSleeping,
        StateCancelling,
        StateProcessing,
        StateCompleting,
        StateCompleted
    };

    core::Atomic<int> state_;
    core::Atomic<int> result_;

    core::nanoseconds_t deadline_;
    core::Seqlock<core::nanoseconds_t> renewed_deadline_;

    core::Atomic<int> renew_in_progress_;
    core::Atomic<int> wait_in_progress_;

    ControlTaskFunc func_;
    core::Atomic<IControlTaskExecutor*> executor_;
    core::Atomic<IControlTaskCompleter*> completer_;

    core::Optional<core::Semaphore> sem_obj_;
    core::Atomic<core::Semaphore*> sem_;
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_CONTROL_TASK_H_
