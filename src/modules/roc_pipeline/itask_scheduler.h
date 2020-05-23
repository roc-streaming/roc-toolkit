/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/itask_scheduler.h
//! @brief Task scheduler interface.

#ifndef ROC_PIPELINE_ITASK_SCHEDULER_H_
#define ROC_PIPELINE_ITASK_SCHEDULER_H_

#include "roc_core/time.h"

namespace roc {
namespace pipeline {

class TaskPipeline;

//! Task scheduler interface.
//! TaskPipeline uses this interface to schedule asynchronous work.
//! ITaskScheduler method calls may come from different threads, but are serialized.
class ITaskScheduler {
public:
    virtual ~ITaskScheduler();

    //! Schedule asynchronous work.
    //!
    //! @p pipeline calls this when it wants to invoke TaskPipeline::process_tasks()
    //! asynchronously.
    //!
    //! @p deadline is a hint when it's better to invoke the method. It's an absolute
    //! timestamp in nanoseconds from the same clock domain as core::timestamp().
    //!
    //! Zero deadline means invoke as soon as possible.
    virtual void schedule_task_processing(TaskPipeline& pipeline,
                                          core::nanoseconds_t deadline) = 0;

    //! Cancel previously scheduled asynchronous work.
    virtual void cancel_task_processing(TaskPipeline& pipeline) = 0;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_ITASK_SCHEDULER_H_
