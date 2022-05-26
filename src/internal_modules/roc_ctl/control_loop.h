/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_ctl/control_loop.h
//! @brief Control loop thread.

#ifndef ROC_CTL_CONTROL_LOOP_H_
#define ROC_CTL_CONTROL_LOOP_H_

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/list.h"
#include "roc_core/shared_ptr.h"
#include "roc_ctl/basic_control_endpoint.h"
#include "roc_ctl/task_queue.h"
#include "roc_pipeline/task_pipeline.h"

namespace roc {
namespace ctl {

//! Control loop thread.
class ControlLoop : public TaskQueue {
public:
    //! Opaque endpoint handle.
    typedef struct EndpointHandle* EndpointHandle;

    //! Control loop task.
    class Task : public TaskQueue::Task {
    protected:
        //! Initialize.
        Task(TaskResult (ControlLoop::*func)(Task&));

    private:
        friend class ControlLoop;

        TaskResult (ControlLoop::*func_)(Task&);

        core::SharedPtr<BasicControlEndpoint> endpoint_;
    };

    //! Subclasses for specific tasks.
    class Tasks {
    public:
        //! Create endpoint on given interface.
        class CreateEndpoint : public Task {
        public:
            //! Set task parameters.
            CreateEndpoint(address::Interface iface, address::Protocol proto);

            //! Get handle of the created endpoint.
            EndpointHandle get_handle() const;

        private:
            friend class ControlLoop;

            address::Interface iface_;
            address::Protocol proto_;
        };

        //! Delete endpoint on given interface of the endpoint set, if it exists.
        class DeleteEndpoint : public Task {
        public:
            //! Set task parameters.
            DeleteEndpoint(EndpointHandle endpoint);
        };

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

    //! Initialize.
    ControlLoop(core::IAllocator& allocator);

    virtual ~ControlLoop();

private:
    virtual TaskResult process_task_imp(TaskQueue::Task&);

    TaskResult task_create_endpoint_(Task&);
    TaskResult task_delete_endpoint_(Task&);
    TaskResult task_process_pipeline_tasks_(Task&);

    core::IAllocator& allocator_;

    core::List<BasicControlEndpoint> endpoints_;
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_CONTROL_LOOP_H_
