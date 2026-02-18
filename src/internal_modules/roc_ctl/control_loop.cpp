/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_ctl/control_loop.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_ctl/control_interface_map.h"

namespace roc {
namespace ctl {

ControlLoop::Tasks::CreateEndpoint::CreateEndpoint(address::Interface iface,
                                                   address::Protocol proto)
    : ControlTask(&ControlLoop::task_create_endpoint_)
    , endpoint_(NULL)
    , iface_(iface)
    , proto_(proto) {
}

ControlLoop::EndpointHandle ControlLoop::Tasks::CreateEndpoint::get_handle() const {
    if (!succeeded()) {
        return NULL;
    }
    roc_panic_if_not(endpoint_);
    return (EndpointHandle)endpoint_.get();
}

ControlLoop::Tasks::DeleteEndpoint::DeleteEndpoint(ControlLoop::EndpointHandle endpoint)
    : ControlTask(&ControlLoop::task_delete_endpoint_)
    , endpoint_((BasicControlEndpoint*)endpoint)
    , phase_(Phase_Prologue) {
}

ControlLoop::Tasks::BindEndpoint::BindEndpoint(EndpointHandle endpoint,
                                               const address::NetworkUri& uri)
    : ControlTask(&ControlLoop::task_bind_endpoint_)
    , endpoint_((BasicControlEndpoint*)endpoint)
    , uri_(uri)
    , phase_(Phase_Prologue) {
}

ControlLoop::Tasks::ConnectEndpoint::ConnectEndpoint(EndpointHandle endpoint,
                                                     const address::NetworkUri& uri)
    : ControlTask(&ControlLoop::task_connect_endpoint_)
    , endpoint_((BasicControlEndpoint*)endpoint)
    , uri_(uri)
    , phase_(Phase_Prologue) {
}

ControlLoop::Tasks::AttachSink::AttachSink(EndpointHandle endpoint,
                                           const address::NetworkUri& uri,
                                           pipeline::SenderLoop& sink)
    : ControlTask(&ControlLoop::task_attach_sink_)
    , endpoint_((BasicControlEndpoint*)endpoint)
    , uri_(uri)
    , sink_(sink) {
}

ControlLoop::Tasks::DetachSink::DetachSink(EndpointHandle endpoint,
                                           pipeline::SenderLoop& sink)
    : ControlTask(&ControlLoop::task_detach_sink_)
    , endpoint_((BasicControlEndpoint*)endpoint)
    , sink_(sink) {
}

ControlLoop::Tasks::AttachSource::AttachSource(EndpointHandle endpoint,
                                               const address::NetworkUri& uri,
                                               pipeline::ReceiverLoop& source)
    : ControlTask(&ControlLoop::task_attach_source_)
    , endpoint_((BasicControlEndpoint*)endpoint)
    , uri_(uri)
    , source_(source) {
}

ControlLoop::Tasks::DetachSource::DetachSource(EndpointHandle endpoint,
                                               pipeline::ReceiverLoop& source)
    : ControlTask(&ControlLoop::task_detach_source_)
    , endpoint_((BasicControlEndpoint*)endpoint)
    , source_(source) {
}

ControlLoop::Tasks::PipelineProcessing::PipelineProcessing(
    pipeline::PipelineLoop& pipeline)
    : ControlTask(&ControlLoop::task_pipeline_processing_)
    , pipeline_(pipeline) {
}

ControlLoop::ControlLoop(netio::NetworkLoop& network_loop, core::IArena& arena)
    : network_loop_(network_loop)
    , arena_(arena) {
}

ControlLoop::~ControlLoop() {
}

status::StatusCode ControlLoop::init_status() const {
    return task_queue_.init_status();
}

void ControlLoop::schedule(ControlTask& task, IControlTaskCompleter* completer) {
    task_queue_.schedule(task, *this, completer);
}

void ControlLoop::schedule_at(ControlTask& task,
                              core::nanoseconds_t deadline,
                              IControlTaskCompleter* completer) {
    task_queue_.schedule_at(task, deadline, *this, completer);
}

bool ControlLoop::schedule_and_wait(ControlTask& task) {
    task_queue_.schedule(task, *this, NULL);
    task_queue_.wait(task);

    return task.succeeded();
}

void ControlLoop::async_cancel(ControlTask& task) {
    task_queue_.async_cancel(task);
}

void ControlLoop::wait(ControlTask& task) {
    task_queue_.wait(task);
}

ControlTaskResult ControlLoop::task_create_endpoint_(ControlTask& control_task) {
    Tasks::CreateEndpoint& task = (Tasks::CreateEndpoint&)control_task;

    roc_log(LogDebug, "control loop: creating endpoint");

    core::SharedPtr<BasicControlEndpoint> endpoint =
        ControlInterfaceMap::instance().new_endpoint(task.iface_, task.proto_,
                                                     task_queue_, network_loop_, arena_);

    if (!endpoint) {
        roc_log(LogError, "control loop: can't add endpoint: failed to create");
        return ControlTaskFailure;
    }

    endpoints_.push_back(*endpoint);
    task.endpoint_ = endpoint;

    return ControlTaskSuccess;
}

ControlTaskResult ControlLoop::task_delete_endpoint_(ControlTask& control_task) {
    Tasks::DeleteEndpoint& task = (Tasks::DeleteEndpoint&)control_task;

    switch (task.phase_) {
    case Tasks::DeleteEndpoint::Phase_Prologue:
        roc_log(LogDebug, "control loop: deleting endpoint");

        if (!endpoints_.contains(*task.endpoint_)) {
            roc_log(LogError, "control loop: can't delete endpoint: endpoint not found");
            return ControlTaskFailure;
        }

        task.endpoint_->async_close(task);

        task.phase_ = Tasks::DeleteEndpoint::Phase_Epilogue;
        return ControlTaskPause;

    case Tasks::DeleteEndpoint::Phase_Epilogue:
        endpoints_.remove(*task.endpoint_);

        return ControlTaskSuccess;
    }

    roc_panic("control loop: invalid phase");
}

ControlTaskResult ControlLoop::task_bind_endpoint_(ControlTask& control_task) {
    Tasks::BindEndpoint& task = (Tasks::BindEndpoint&)control_task;

    switch (task.phase_) {
    case Tasks::BindEndpoint::Phase_Prologue:
        if (!endpoints_.contains(*task.endpoint_)) {
            roc_log(LogError, "control loop: can't bind endpoint: endpoint not found");
            return ControlTaskFailure;
        }

        if (!task.endpoint_->async_bind(task.uri_, task)) {
            roc_log(LogError, "control loop: can't bind endpoint");
            return ControlTaskFailure;
        }

        task.phase_ = Tasks::BindEndpoint::Phase_Epilogue;
        return ControlTaskPause;

    case Tasks::BindEndpoint::Phase_Epilogue:
        if (!task.endpoint_->is_bound()) {
            roc_log(LogError, "control loop: can't bind endpoint");
            return ControlTaskFailure;
        }

        return ControlTaskSuccess;
    }

    roc_panic("control loop: invalid phase");
}

ControlTaskResult ControlLoop::task_connect_endpoint_(ControlTask& control_task) {
    Tasks::ConnectEndpoint& task = (Tasks::ConnectEndpoint&)control_task;

    switch (task.phase_) {
    case Tasks::ConnectEndpoint::Phase_Prologue:
        if (!endpoints_.contains(*task.endpoint_)) {
            roc_log(LogError, "control loop: can't connect endpoint: endpoint not found");
            return ControlTaskFailure;
        }

        if (!task.endpoint_->async_connect(task.uri_, task)) {
            roc_log(LogError, "control loop: can't connect endpoint");
            return ControlTaskFailure;
        }

        task.phase_ = Tasks::ConnectEndpoint::Phase_Epilogue;
        return ControlTaskPause;

    case Tasks::ConnectEndpoint::Phase_Epilogue:
        if (!task.endpoint_->is_connected()) {
            roc_log(LogError, "control loop: can't connect endpoint");
            return ControlTaskFailure;
        }

        return ControlTaskSuccess;
    }

    roc_panic("control loop: invalid phase");
}

ControlTaskResult ControlLoop::task_attach_sink_(ControlTask& control_task) {
    Tasks::AttachSink& task = (Tasks::AttachSink&)control_task;

    if (!endpoints_.contains(*task.endpoint_)) {
        roc_log(LogError, "control loop: can't attach sink: endpoint not found");
        return ControlTaskFailure;
    }

    if (!task.endpoint_->attach_sink(task.uri_, task.sink_)) {
        roc_log(LogError, "control loop: can't attach sink: attach failed");
        return ControlTaskFailure;
    }

    return ControlTaskSuccess;
}

ControlTaskResult ControlLoop::task_detach_sink_(ControlTask& control_task) {
    Tasks::DetachSink& task = (Tasks::DetachSink&)control_task;

    if (!endpoints_.contains(*task.endpoint_)) {
        roc_log(LogError, "control loop: can't detach sink: endpoint not found");
        return ControlTaskFailure;
    }

    if (!task.endpoint_->detach_sink(task.sink_)) {
        roc_log(LogError, "control loop: can't detach sink: detach failed");
        return ControlTaskFailure;
    }

    return ControlTaskSuccess;
}

ControlTaskResult ControlLoop::task_attach_source_(ControlTask& control_task) {
    Tasks::AttachSource& task = (Tasks::AttachSource&)control_task;

    if (!endpoints_.contains(*task.endpoint_)) {
        roc_log(LogError, "control loop: can't attach source: endpoint not found");
        return ControlTaskFailure;
    }

    if (!task.endpoint_->attach_source(task.uri_, task.source_)) {
        roc_log(LogError, "control loop: can't attach source: attach failed");
        return ControlTaskFailure;
    }

    return ControlTaskSuccess;
}

ControlTaskResult ControlLoop::task_detach_source_(ControlTask& control_task) {
    Tasks::DetachSource& task = (Tasks::DetachSource&)control_task;

    if (!endpoints_.contains(*task.endpoint_)) {
        roc_log(LogError, "control loop: can't detach source: endpoint not found");
        return ControlTaskFailure;
    }

    if (!task.endpoint_->detach_source(task.source_)) {
        roc_log(LogError, "control loop: can't detach source: detach failed");
        return ControlTaskFailure;
    }

    return ControlTaskSuccess;
}

ControlTaskResult ControlLoop::task_pipeline_processing_(ControlTask& control_task) {
    Tasks::PipelineProcessing& task = (Tasks::PipelineProcessing&)control_task;

    task.pipeline_.process_tasks();

    return ControlTaskSuccess;
}

} // namespace ctl
} // namespace roc
