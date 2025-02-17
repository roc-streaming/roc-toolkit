/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <benchmark/benchmark.h>

#include "roc_core/fast_random.h"
#include "roc_core/heap_arena.h"
#include "roc_ctl/control_task_executor.h"
#include "roc_ctl/control_task_queue.h"
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
    BatchSize = 10000,
    FrameBufSize = 100
};

core::HeapArena arena;

core::SlabPool<audio::Frame> frame_pool("frame_pool", arena);
core::SlabPool<core::Buffer> frame_buffer_pool("frame_buffer_pool", arena, FrameBufSize);

class NoopPipeline : public PipelineLoop,
                     private IPipelineTaskScheduler,
                     public ctl::ControlTaskExecutor<NoopPipeline> {
public:
    struct Task : PipelineTask {
        Task() {
        }
    };

    NoopPipeline(const PipelineLoopConfig& config, ctl::ControlTaskQueue& control_queue)
        : PipelineLoop(*this,
                       config,
                       audio::SampleSpec(SampleRate,
                                         audio::PcmSubformat_Raw,
                                         audio::ChanLayout_Surround,
                                         audio::ChanOrder_Smpte,
                                         Chans),
                       frame_pool,
                       frame_buffer_pool,
                       Dir_WriteFrames)
        , control_queue_(control_queue)
        , control_task_(*this) {
    }

    ~NoopPipeline() {
        control_queue_.wait(control_task_);
    }

    void stop_and_wait() {
        control_queue_.async_cancel(control_task_);

        while (num_pending_tasks() != 0) {
            process_tasks();
        }
    }

private:
    struct BackgroundProcessingTask : ctl::ControlTask {
        BackgroundProcessingTask(NoopPipeline& p)
            : ControlTask(&NoopPipeline::do_processing_)
            , pipeline(p) {
        }

        NoopPipeline& pipeline;
    };

    virtual core::nanoseconds_t timestamp_imp() const {
        return core::timestamp(core::ClockMonotonic);
    }

    virtual uint64_t tid_imp() const {
        return 0;
    }

    virtual status::StatusCode process_subframe_imp(audio::Frame& frame,
                                                    packet::stream_timestamp_t duration,
                                                    audio::FrameReadMode mode) {
        return status::StatusOK;
    }

    virtual bool process_task_imp(PipelineTask& task) {
        return true;
    }

    virtual void schedule_task_processing(PipelineLoop&, core::nanoseconds_t deadline) {
        control_queue_.schedule_at(control_task_, deadline, *this, NULL);
    }

    virtual void cancel_task_processing(PipelineLoop&) {
        control_queue_.async_cancel(control_task_);
    }

    ctl::ControlTaskResult do_processing_(ctl::ControlTask& task) {
        ((BackgroundProcessingTask&)task).pipeline.process_tasks();
        return ctl::ControlTaskSuccess;
    }

    ctl::ControlTaskQueue& control_queue_;
    BackgroundProcessingTask control_task_;
};

class NoopCompleter : public IPipelineTaskCompleter {
public:
    virtual void pipeline_task_completed(PipelineTask&) {
    }
};

struct BM_PipelineContention : benchmark::Fixture {
    ctl::ControlTaskQueue control_queue;

    PipelineLoopConfig config;

    NoopPipeline pipeline;
    NoopCompleter completer;

    BM_PipelineContention()
        : control_queue()
        , pipeline(config, control_queue) {
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
