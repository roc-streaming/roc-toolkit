/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <benchmark/benchmark.h>

#include "roc_core/fast_random.h"
#include "roc_core/heap_allocator.h"
#include "roc_ctl/control_loop.h"
#include "roc_pipeline/pipeline_loop.h"

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

core::HeapAllocator allocator;

class NoopPipeline : public PipelineLoop, private IPipelineTaskScheduler {
public:
    class Task : public PipelineTask {
    public:
        Task() {
        }
    };

    NoopPipeline(const TaskConfig& config, ctl::ControlLoop& loop)
        : PipelineLoop(*this, config, audio::SampleSpec(SampleRate, Chans))
        , loop_(loop)
        , processing_task_(*this) {
    }

    ~NoopPipeline() {
        loop_.wait(processing_task_);
    }

    void stop_and_wait() {
        loop_.async_cancel(processing_task_);

        while (num_pending_tasks() != 0) {
            process_tasks();
        }
    }

private:
    virtual core::nanoseconds_t timestamp_imp() const {
        return core::timestamp();
    }

    virtual bool process_subframe_imp(audio::Frame&) {
        return true;
    }

    virtual bool process_task_imp(PipelineTask&) {
        return true;
    }

    virtual void schedule_task_processing(PipelineLoop&, core::nanoseconds_t deadline) {
        loop_.schedule_at(processing_task_, deadline, NULL);
    }

    virtual void cancel_task_processing(PipelineLoop&) {
        loop_.async_cancel(processing_task_);
    }

    ctl::ControlLoop& loop_;
    ctl::ControlLoop::Tasks::PipelineProcessing processing_task_;
};

class NoopCompleter : public IPipelineTaskCompleter {
public:
    virtual void pipeline_task_completed(PipelineTask&) {
    }
};

struct BM_PipelineContention : benchmark::Fixture {
    ctl::ControlLoop ctl_loop;

    TaskConfig config;

    NoopPipeline pipeline;
    NoopCompleter completer;

    BM_PipelineContention()
        : ctl_loop(allocator)
        , pipeline(config, ctl_loop) {
    }
};

BENCHMARK_DEFINE_F(BM_PipelineContention, Schedule)(benchmark::State& state) {
    NoopPipeline::Task* tasks = new NoopPipeline::Task[NumIterations];
    size_t n_task = 0;

    while (state.KeepRunningBatch(BatchSize)) {
        for (int n = 0; n < BatchSize; n++) {
            pipeline.schedule(tasks[n_task++], completer);
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
