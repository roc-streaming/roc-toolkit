/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_SCHEDULER_H_
#define ROC_PIPELINE_TEST_HELPERS_SCHEDULER_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/mutex.h"
#include "roc_ctl/control_loop.h"
#include "roc_pipeline/itask_scheduler.h"

namespace roc {
namespace pipeline {
namespace test {

class Scheduler : public pipeline::ITaskScheduler {
public:
    Scheduler()
        : task_(NULL) {
        CHECK(loop_.valid());
    }

    ~Scheduler() {
        if (task_) {
            FAIL("stop_and_wait() was not called before desctructor");
        }
    }

    void wait_done() {
        ctl::ControlLoop::Tasks::ProcessPipelineTasks* task = NULL;

        {
            core::Mutex::Lock lock(mutex_);
            task = task_;
        }

        if (task) {
            loop_.wait(*task);

            core::Mutex::Lock lock(mutex_);

            delete task_;
            task_ = NULL;
        }
    }

    virtual void schedule_task_processing(pipeline::TaskPipeline& pipeline,
                                          core::nanoseconds_t delay) {
        core::Mutex::Lock lock(mutex_);
        if (!task_) {
            task_ = new ctl::ControlLoop::Tasks::ProcessPipelineTasks(pipeline);
        }
        loop_.reschedule_after(*task_, delay);
    }

    virtual void cancel_task_processing(pipeline::TaskPipeline&) {
        core::Mutex::Lock lock(mutex_);
        if (task_) {
            loop_.async_cancel(*task_);
        }
    }

private:
    core::Mutex mutex_;
    ctl::ControlLoop loop_;
    ctl::ControlLoop::Tasks::ProcessPipelineTasks* task_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_SCHEDULER_H_
