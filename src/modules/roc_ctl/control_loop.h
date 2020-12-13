/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_ctl/control_loop.h
//! @brief Control loop thread.

#ifndef ROC_CTL_CONTROL_LOOP_H_
#define ROC_CTL_CONTROL_LOOP_H_

#include "roc_ctl/task_queue.h"
#include "roc_pipeline/task_pipeline.h"

namespace roc {
namespace ctl {

//! Control loop thread.
class ControlLoop : public TaskQueue {
public:
    //! Control loop task.
    class Task : public TaskQueue::Task {
    protected:
        //! Initialize.
        Task(TaskResult (ControlLoop::*func)(Task&));

    private:
        friend class ControlLoop;

        TaskResult (ControlLoop::*func_)(Task&);
    };

    //! Subclasses for specific tasks.
    class Tasks {
    public:
        //! Process pending pipeline tasks.
        class ProcessPipelineTasks : public Task {
        public:
            //! Set task parameters.
            ProcessPipelineTasks(pipeline::TaskPipeline& pipeline);

        private:
            friend class ControlLoop;

            pipeline::TaskPipeline& pipeline_;
        };
    };

    virtual ~ControlLoop();

private:
    virtual TaskResult process_task_imp(TaskQueue::Task&);
    virtual core::nanoseconds_t timestamp_imp() const;
    TaskResult task_process_pipeline_tasks_(Task&);
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_CONTROL_LOOP_H_
