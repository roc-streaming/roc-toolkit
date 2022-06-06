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
#include "roc_core/noncopyable.h"
#include "roc_core/shared_ptr.h"
#include "roc_ctl/basic_control_endpoint.h"
#include "roc_ctl/control_task_executor.h"
#include "roc_ctl/control_task_queue.h"
#include "roc_pipeline/pipeline_loop.h"

namespace roc {
namespace ctl {

//! Control loop thread.
//! @remarks
//!  This class is a task-based facade for the whole roc_ctl module.
class ControlLoop : public ControlTaskExecutor<ControlLoop>, public core::NonCopyable<> {
public:
    //! Opaque endpoint handle.
    typedef struct EndpointHandle* EndpointHandle;

    //! Subclasses for specific tasks.
    class Tasks {
    public:
        //! Create endpoint on given interface.
        class CreateEndpoint : public ControlTask {
        public:
            //! Set task parameters.
            CreateEndpoint(address::Interface iface, address::Protocol proto);

            //! Get handle of the created endpoint.
            EndpointHandle get_handle() const;

        private:
            friend class ControlLoop;

            core::SharedPtr<BasicControlEndpoint> endpoint_;
            address::Interface iface_;
            address::Protocol proto_;
        };

        //! Delete endpoint on given interface of the endpoint set, if it exists.
        class DeleteEndpoint : public ControlTask {
        public:
            //! Set task parameters.
            DeleteEndpoint(EndpointHandle endpoint);

        private:
            friend class ControlLoop;

            core::SharedPtr<BasicControlEndpoint> endpoint_;
        };

        //! Help pipeline to process its pending tasks.
        class PipelineProcessing : public ControlTask {
        public:
            //! Set task parameters.
            PipelineProcessing(pipeline::PipelineLoop& pipeline);

        private:
            friend class ControlLoop;

            pipeline::PipelineLoop& pipeline_;
        };
    };

    //! Initialize.
    ControlLoop(core::IAllocator& allocator);

    virtual ~ControlLoop();

    //! Check if the object was successfully constructed.
    bool valid() const;

    //! Enqueue a task for asynchronous execution in the nearest future.
    //! @see ControlTaskQueue::schedule.
    void schedule(ControlTask& task, IControlTaskCompleter* completer);

    //! Enqueue a task for asynchronous execution at given point of time.
    //! @see ControlTaskQueue::schedule_at.
    void schedule_at(ControlTask& task,
                     core::nanoseconds_t deadline,
                     IControlTaskCompleter* completer);

    //! Cancel scheduled task execution.
    //! @see ControlTaskQueue::async_cancel.
    void async_cancel(ControlTask& task);

    //! Wait until the task is finished.
    //! @see ControlTaskQueue::wait.
    void wait(ControlTask& task);

private:
    ControlTaskResult task_create_endpoint_(ControlTask&);
    ControlTaskResult task_delete_endpoint_(ControlTask&);
    ControlTaskResult task_process_pipeline_tasks_(ControlTask&);

    core::IAllocator& allocator_;

    ControlTaskQueue task_queue_;

    core::List<BasicControlEndpoint> endpoints_;
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_CONTROL_LOOP_H_
