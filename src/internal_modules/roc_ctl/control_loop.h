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
#include "roc_core/attributes.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/shared_ptr.h"
#include "roc_ctl/basic_control_endpoint.h"
#include "roc_ctl/control_task_executor.h"
#include "roc_ctl/control_task_queue.h"
#include "roc_netio/network_loop.h"
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

        //! Delete endpoint, if it exists.
        class DeleteEndpoint : public ControlTask {
        public:
            //! Set task parameters.
            DeleteEndpoint(EndpointHandle endpoint);

        private:
            friend class ControlLoop;

            enum Phase { Phase_Prologue, Phase_Epilogue };

            core::SharedPtr<BasicControlEndpoint> endpoint_;
            Phase phase_;
        };

        //! Bind endpoint on local URI.
        class BindEndpoint : public ControlTask {
        public:
            //! Set task parameters.
            BindEndpoint(EndpointHandle endpoint, const address::NetworkUri& uri);

        private:
            friend class ControlLoop;

            enum Phase { Phase_Prologue, Phase_Epilogue };

            core::SharedPtr<BasicControlEndpoint> endpoint_;
            const address::NetworkUri& uri_;
            Phase phase_;
        };

        //! Connect endpoint on remote URI.
        class ConnectEndpoint : public ControlTask {
        public:
            //! Set task parameters.
            ConnectEndpoint(EndpointHandle endpoint, const address::NetworkUri& uri);

        private:
            friend class ControlLoop;

            enum Phase { Phase_Prologue, Phase_Epilogue };

            core::SharedPtr<BasicControlEndpoint> endpoint_;
            const address::NetworkUri& uri_;
            Phase phase_;
        };

        //! Attach sink to endpoint at given URI.
        class AttachSink : public ControlTask {
        public:
            //! Set task parameters.
            AttachSink(EndpointHandle endpoint,
                       const address::NetworkUri& uri,
                       pipeline::SenderLoop& sink);

        private:
            friend class ControlLoop;

            core::SharedPtr<BasicControlEndpoint> endpoint_;
            const address::NetworkUri& uri_;
            pipeline::SenderLoop& sink_;
        };

        //! Detach sink from endpoint.
        class DetachSink : public ControlTask {
        public:
            //! Set task parameters.
            DetachSink(EndpointHandle endpoint, pipeline::SenderLoop& sink);

        private:
            friend class ControlLoop;

            core::SharedPtr<BasicControlEndpoint> endpoint_;
            pipeline::SenderLoop& sink_;
        };

        //! Attach source to endpoint at given URI.
        class AttachSource : public ControlTask {
        public:
            //! Set task parameters.
            AttachSource(EndpointHandle endpoint,
                         const address::NetworkUri& uri,
                         pipeline::ReceiverLoop& source);

        private:
            friend class ControlLoop;

            core::SharedPtr<BasicControlEndpoint> endpoint_;
            const address::NetworkUri& uri_;
            pipeline::ReceiverLoop& source_;
        };

        //! Detach source from endpoint.
        class DetachSource : public ControlTask {
        public:
            //! Set task parameters.
            DetachSource(EndpointHandle endpoint, pipeline::ReceiverLoop& source);

        private:
            friend class ControlLoop;

            core::SharedPtr<BasicControlEndpoint> endpoint_;
            pipeline::ReceiverLoop& source_;
        };

        //! Process pending pipeline tasks on control thread.
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
    ControlLoop(netio::NetworkLoop& network_loop, core::IArena& arena);

    virtual ~ControlLoop();

    //! Check if control loop was successfully constructed.
    status::StatusCode init_status() const;

    //! Enqueue a task for asynchronous execution as soon as possible.
    //! @p completer will be invoked on control thread when the task completes.
    //! @see ControlTaskQueue::schedule for details.
    void schedule(ControlTask& task, IControlTaskCompleter* completer);

    //! Enqueue a task for asynchronous execution at given point of time.
    //! @p deadline defines the absolute point of time when to execute the task.
    //! @p completer will be invoked on control thread when the task completes.
    //! @see ControlTaskQueue::schedule_at for details.
    void schedule_at(ControlTask& task,
                     core::nanoseconds_t deadline,
                     IControlTaskCompleter* completer);

    //! Enqueue a task for asynchronous execution and wait until it completes.
    //! Combines schedule() and wait() calls.
    //! @returns
    //!  true if the task succeeded or false if it failed.
    ROC_ATTR_NODISCARD bool schedule_and_wait(ControlTask& task);

    //! Try to cancel scheduled task execution, if it's not executed yet.
    //! @see ControlTaskQueue::async_cancel for details.
    void async_cancel(ControlTask& task);

    //! Wait until the task is completed.
    //! @see ControlTaskQueue::wait for details.
    void wait(ControlTask& task);

private:
    ControlTaskResult task_create_endpoint_(ControlTask&);
    ControlTaskResult task_delete_endpoint_(ControlTask&);
    ControlTaskResult task_bind_endpoint_(ControlTask&);
    ControlTaskResult task_connect_endpoint_(ControlTask&);
    ControlTaskResult task_attach_sink_(ControlTask&);
    ControlTaskResult task_detach_sink_(ControlTask&);
    ControlTaskResult task_attach_source_(ControlTask&);
    ControlTaskResult task_detach_source_(ControlTask&);
    ControlTaskResult task_pipeline_processing_(ControlTask&);

    netio::NetworkLoop& network_loop_;
    core::IArena& arena_;

    ControlTaskQueue task_queue_;

    core::List<BasicControlEndpoint> endpoints_;
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_CONTROL_LOOP_H_
