/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/pipeline_task.h
//! @brief Base class for pipeline tasks.

#ifndef ROC_PIPELINE_PIPELINE_TASK_H_
#define ROC_PIPELINE_PIPELINE_TASK_H_

#include "roc_core/atomic.h"
#include "roc_core/mpsc_queue_node.h"
#include "roc_core/optional.h"
#include "roc_core/semaphore.h"

namespace roc {
namespace pipeline {

class PipelineLoop;
class IPipelineTaskCompleter;

//! Base class for pipeline tasks.
class PipelineTask : public core::MpscQueueNode<> {
public:
    ~PipelineTask();

    //! Check that the task finished and succeeded.
    bool success() const;

protected:
    PipelineTask();

private:
    friend class PipelineLoop;

    enum { StateNew, StateScheduled, StateFinished };

    // Task state, defines whether task is finished already.
    // The task becomes immutable after setting state_ to StateFinished;
    core::Atomic<int> state_;

    // Task result, defines wether finished task succeeded or failed.
    // Makes sense only after setting state_ to StateFinished.
    // This atomic should be assigned before setting state_ to StateFinished.
    core::Atomic<int> success_;

    // Completion handler;
    IPipelineTaskCompleter* completer_;

    // Completion semaphore.
    core::Optional<core::Semaphore> sem_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_PIPELINE_TASK_H_
