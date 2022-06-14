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
    , endpoint_((BasicControlEndpoint*)endpoint) {
}

ControlLoop::Tasks::PipelineProcessing::PipelineProcessing(
    pipeline::PipelineLoop& pipeline)
    : ControlTask(&ControlLoop::task_process_pipeline_tasks_)
    , pipeline_(pipeline) {
}

ControlLoop::ControlLoop(core::IAllocator& allocator)
    : allocator_(allocator) {
}

ControlLoop::~ControlLoop() {
}

bool ControlLoop::valid() const {
    return task_queue_.valid();
}

void ControlLoop::schedule(ControlTask& task, IControlTaskCompleter* completer) {
    task_queue_.schedule(task, *this, completer);
}

void ControlLoop::schedule_at(ControlTask& task,
                              core::nanoseconds_t deadline,
                              IControlTaskCompleter* completer) {
    task_queue_.schedule_at(task, deadline, *this, completer);
}

void ControlLoop::async_cancel(ControlTask& task) {
    task_queue_.async_cancel(task);
}

void ControlLoop::wait(ControlTask& task) {
    task_queue_.wait(task);
}

ControlTaskResult ControlLoop::task_create_endpoint_(ControlTask& control_task) {
    Tasks::CreateEndpoint& task = (Tasks::CreateEndpoint&)control_task;

    core::SharedPtr<BasicControlEndpoint> endpoint =
        ControlInterfaceMap::instance().new_endpoint(task.iface_, task.proto_,
                                                     allocator_);

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

    if (!endpoints_.contains(*task.endpoint_)) {
        roc_log(LogError, "control loop: can't delete endpoint: not added");
        return ControlTaskFailure;
    }

    task.endpoint_->close();
    endpoints_.remove(*task.endpoint_);

    return ControlTaskSuccess;
}

ControlTaskResult ControlLoop::task_process_pipeline_tasks_(ControlTask& control_task) {
    Tasks::PipelineProcessing& task = (Tasks::PipelineProcessing&)control_task;

    task.pipeline_.process_tasks();

    return ControlTaskSuccess;
}

} // namespace ctl
} // namespace roc
