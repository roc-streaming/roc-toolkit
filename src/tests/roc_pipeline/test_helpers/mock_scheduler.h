/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_MOCK_SCHEDULER_H_
#define ROC_PIPELINE_TEST_HELPERS_MOCK_SCHEDULER_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/mutex.h"
#include "roc_ctl/control_task_executor.h"
#include "roc_ctl/control_task_queue.h"
#include "roc_pipeline/ipipeline_task_scheduler.h"
#include "roc_pipeline/pipeline_loop.h"

namespace roc {
namespace pipeline {
namespace test {

class MockScheduler : public IPipelineTaskScheduler,
                      public ctl::ControlTaskExecutor<MockScheduler> {
    class ProcessingTask : public ctl::ControlTask {
    public:
        ProcessingTask(PipelineLoop& pipeline)
            : ControlTask(&MockScheduler::do_processing_)
            , pipeline_(pipeline) {
        }

    private:
        friend class MockScheduler;

        PipelineLoop& pipeline_;
    };

public:
    MockScheduler()
        : task_(NULL) {
        LONGS_EQUAL(status::StatusOK, queue_.init_status());
    }

    ~MockScheduler() {
        if (task_) {
            FAIL("wait_done() was not called before destructor");
        }
    }

    void wait_done() {
        ProcessingTask* task = NULL;

        {
            core::Mutex::Lock lock(mutex_);
            task = task_;
        }

        if (task) {
            queue_.wait(*task);

            core::Mutex::Lock lock(mutex_);

            delete task_;
            task_ = NULL;
        }
    }

    virtual void schedule_task_processing(PipelineLoop& pipeline,
                                          core::nanoseconds_t deadline) {
        core::Mutex::Lock lock(mutex_);
        if (!task_) {
            task_ = new ProcessingTask(pipeline);
        }
        queue_.schedule_at(*task_, deadline, *this, NULL);
    }

    virtual void cancel_task_processing(PipelineLoop&) {
        core::Mutex::Lock lock(mutex_);
        if (task_) {
            queue_.async_cancel(*task_);
        }
    }

private:
    ctl::ControlTaskResult do_processing_(ctl::ControlTask& task) {
        ((ProcessingTask&)task).pipeline_.process_tasks();
        return ctl::ControlTaskSuccess;
    }

    core::Mutex mutex_;
    ctl::ControlTaskQueue queue_;
    ProcessingTask* task_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_MOCK_SCHEDULER_H_
