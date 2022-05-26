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

ControlLoop::Task::Task(TaskResult (ControlLoop::*func)(Task&))
    : func_(func)
    , endpoint_(NULL) {
}

ControlLoop::Tasks::CreateEndpoint::CreateEndpoint(address::Interface iface,
                                                   address::Protocol proto)
    : Task(&ControlLoop::task_create_endpoint_)
    , iface_(iface)
    , proto_(proto) {
}

ControlLoop::EndpointHandle ControlLoop::Tasks::CreateEndpoint::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(endpoint_);
    return (EndpointHandle)endpoint_.get();
}

ControlLoop::Tasks::DeleteEndpoint::DeleteEndpoint(ControlLoop::EndpointHandle endpoint)
    : Task(&ControlLoop::task_delete_endpoint_) {
    endpoint_ = (BasicControlEndpoint*)endpoint;
}

ControlLoop::Tasks::ProcessPipelineTasks::ProcessPipelineTasks(
    pipeline::TaskPipeline& pipeline)
    : Task(&ControlLoop::task_process_pipeline_tasks_)
    , pipeline_(pipeline) {
}

ControlLoop::ControlLoop(core::IAllocator& allocator)
    : allocator_(allocator) {
}

ControlLoop::~ControlLoop() {
    stop_and_wait();
}

TaskQueue::TaskResult ControlLoop::process_task_imp(TaskQueue::Task& basic_task) {
    Task& task = (Task&)basic_task;
    return (this->*(task.func_))(task);
}

TaskQueue::TaskResult ControlLoop::task_create_endpoint_(Task& basic_task) {
    Tasks::CreateEndpoint& task = (Tasks::CreateEndpoint&)basic_task;

    core::SharedPtr<BasicControlEndpoint> endpoint =
        ControlInterfaceMap::instance().new_endpoint(task.iface_, task.proto_,
                                                     allocator_);

    if (!endpoint) {
        roc_log(LogError, "control loop: can't add endpoint: failed to create");
        return TaskFailed;
    }

    endpoints_.push_back(*endpoint);
    task.endpoint_ = endpoint;

    return TaskSucceeded;
}

TaskQueue::TaskResult ControlLoop::task_delete_endpoint_(Task& basic_task) {
    Tasks::DeleteEndpoint& task = (Tasks::DeleteEndpoint&)basic_task;

    if (!endpoints_.contains(*task.endpoint_)) {
        roc_log(LogError, "control loop: can't delete endpoint: not added");
        return TaskFailed;
    }

    task.endpoint_->close();
    endpoints_.remove(*task.endpoint_);

    return TaskSucceeded;
}

TaskQueue::TaskResult ControlLoop::task_process_pipeline_tasks_(Task& basic_task) {
    Tasks::ProcessPipelineTasks& task = (Tasks::ProcessPipelineTasks&)basic_task;

    task.pipeline_.process_tasks();

    return TaskSucceeded;
}

} // namespace ctl
} // namespace roc
