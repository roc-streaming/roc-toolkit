/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_ctl/control_loop.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace ctl {

ControlLoop::Task::Task(TaskResult (ControlLoop::*func)(Task&))
    : func_(func) {
}

ControlLoop::Tasks::ProcessPipelineTasks::ProcessPipelineTasks(
    pipeline::TaskPipeline& pipeline)
    : Task(&ControlLoop::task_process_pipeline_tasks_)
    , pipeline_(pipeline) {
}

ControlLoop::~ControlLoop() {
    stop_and_wait();
}

TaskQueue::TaskResult ControlLoop::process_task_imp(TaskQueue::Task& basic_task) {
    Task& task = (Task&)basic_task;
    return (this->*(task.func_))(task);
}

core::nanoseconds_t ControlLoop::timestamp_imp() const {
    return core::timestamp();
}


TaskQueue::TaskResult ControlLoop::task_process_pipeline_tasks_(Task& basic_task) {
    Tasks::ProcessPipelineTasks& task = (Tasks::ProcessPipelineTasks&)basic_task;

    task.pipeline_.process_tasks();

    return TaskSucceeded;
}

} // namespace ctl
} // namespace roc
