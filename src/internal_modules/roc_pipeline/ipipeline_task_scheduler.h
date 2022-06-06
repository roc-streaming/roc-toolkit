/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/ipipeline_task_scheduler.h
//! @brief Pipeline task scheduler interface.

#ifndef ROC_PIPELINE_IPIPELINE_TASK_SCHEDULER_H_
#define ROC_PIPELINE_IPIPELINE_TASK_SCHEDULER_H_

#include "roc_core/time.h"

namespace roc {
namespace pipeline {

class PipelineLoop;

//! Pipeline task scheduler interface.
//! PipelineLoop uses this interface to schedule asynchronous work.
//! Method calls may come from different threads, but are serialized.
class IPipelineTaskScheduler {
public:
    virtual ~IPipelineTaskScheduler();

    //! Schedule asynchronous work.
    //!
    //! @p pipeline calls this when it wants to invoke PipelineLoop::process_tasks()
    //! asynchronously.
    //!
    //! @p deadline is a hint when it's better to invoke the method. It's an absolute
    //! timestamp in nanoseconds from the same clock domain as core::timestamp().
    //!
    //! Zero deadline means invoke as soon as possible.
    virtual void schedule_task_processing(PipelineLoop& pipeline,
                                          core::nanoseconds_t deadline) = 0;

    //! Cancel previously scheduled asynchronous work.
    virtual void cancel_task_processing(PipelineLoop& pipeline) = 0;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_IPIPELINE_TASK_SCHEDULER_H_
