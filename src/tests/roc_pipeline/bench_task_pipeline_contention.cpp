/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <benchmark/benchmark.h>

#include "roc_core/fast_random.h"
#include "roc_ctl/control_loop.h"
#include "roc_pipeline/task_pipeline.h"

namespace roc {
namespace pipeline {
namespace {

// This benchmarks starts a few threads using the same pipeline and measures
// scheduling times under contention.
//
// It allows to ensure that the scheduling time does not depend on the
// contention level, i.e. the number of threads running.
//
// Note that the scheduling time for one-thread run is higher because the
// pipeline is able to perform in-place task execution in this case and the
// scheduling time also includes task execution time.

enum {
    SampleRate = 1000000, // 1 sample = 1 us (for convenience)
    Chans = 0x1,
    NumThreads = 16,
    NumIterations = 1000000,
    BatchSize = 10000
};

class NoopPipeline : public TaskPipeline, private ITaskScheduler {
public:
    class Task : public TaskPipeline::Task {
    public:
        Task() {
        }
    };

    NoopPipeline(const TaskConfig& config, ctl::ControlLoop& loop)
        : TaskPipeline(*this, config, SampleRate, Chans)
        , loop_(loop)
        , process_tasks_(*this) {
    }

    ~NoopPipeline() {
        loop_.wait(process_tasks_);
    }

    void stop_and_wait() {
        loop_.async_cancel(process_tasks_);

        while (num_pending_tasks() != 0) {
            process_tasks();
        }
    }

private:
    virtual core::nanoseconds_t timestamp_imp() const {
        return core::timestamp();
    }

    virtual bool process_frame_imp(audio::Frame&) {
        return true;
    }

    virtual bool process_task_imp(TaskPipeline::Task&) {
        return true;
    }

    virtual void schedule_task_processing(TaskPipeline&, core::nanoseconds_t deadline) {
        loop_.reschedule_at(process_tasks_, deadline);
    }

    virtual void cancel_task_processing(TaskPipeline&) {
        loop_.async_cancel(process_tasks_);
    }

    ctl::ControlLoop& loop_;
    ctl::ControlLoop::Tasks::ProcessPipelineTasks process_tasks_;
};

class NoopHandler : public TaskPipeline::ICompletionHandler {
public:
    virtual void pipeline_task_finished(TaskPipeline::Task&) {
    }
};

struct BM_PipelineContention : benchmark::Fixture {
    ctl::ControlLoop ctl_loop;

    TaskConfig config;

    NoopPipeline pipeline;
    NoopHandler handler;

    BM_PipelineContention()
        : pipeline(config, ctl_loop) {
    }
};

BENCHMARK_DEFINE_F(BM_PipelineContention, Schedule)(benchmark::State& state) {
    NoopPipeline::Task* tasks = new NoopPipeline::Task[NumIterations];
    size_t n_task = 0;

    while (state.KeepRunningBatch(BatchSize)) {
        for (int n = 0; n < BatchSize; n++) {
            pipeline.schedule(tasks[n_task++], handler);
        }
    }

    pipeline.stop_and_wait();

    delete[] tasks;
}

BENCHMARK_REGISTER_F(BM_PipelineContention, Schedule)
    ->ThreadRange(1, NumThreads)
    ->Iterations(NumIterations)
    ->Unit(benchmark::kMicrosecond);

} // namespace
} // namespace pipeline
} // namespace roc
