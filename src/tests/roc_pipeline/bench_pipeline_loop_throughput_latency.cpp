/*
 * Copyright (c) Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <benchmark/benchmark.h>
#include "bench_helper.h"

#include "roc_core/atomic_bool.h"
#include "roc_core/fast_random.h"
#include "roc_core/heap_arena.h"
#include "roc_core/stddefs.h"
#include "roc_core/thread.h"
#include "roc_core/ticker.h"
#include "roc_core/time.h"
#include "roc_ctl/control_task_executor.h"
#include "roc_ctl/control_task_queue.h"
#include "roc_pipeline/pipeline_loop.h"

namespace roc {
namespace pipeline {
namespace {

// --------
// Overview
// --------
//
// This benchmark measures:
//   1. Task throughput - how many tasks per second can the pipeline process
//   2. Task latency - p95 delay between scheduling a task and processing it
//
// We test different combinations of:
//   - Frame sizes: 1ms (small), 5ms (medium), 20ms (large)
//   - Processing load: 5% (light) and 80% (heavy) of frame duration
//
// Frame processing is simulated with busy-loop (NOT sleep!) to ensure
// Linux scheduler treats us as CPU-bound thread, matching real audio processing.

enum {
    SampleRate = 1000000, // 1 sample = 1 us (for convenience)
    Chans = 0x1,
    NumIterations = 1000
};

// Frame durations
const core::nanoseconds_t FrameDuration_1ms = 1 * core::Millisecond;
const core::nanoseconds_t FrameDuration_5ms = 5 * core::Millisecond;
const core::nanoseconds_t FrameDuration_20ms = 20 * core::Millisecond;

// Processing ratios
const double ProcessingRatio_Light = 0.05; // 5%
const double ProcessingRatio_Heavy = 0.80; // 80%

// Task processing time
const core::nanoseconds_t TaskProcessingDuration = 5 * core::Microsecond;

core::HeapArena arena;

core::SlabPool<audio::Frame> frame_pool("frame_pool", arena);

class BenchStats {
public:
    void reset() {
        task_latency_ = roc::helper::Counter();
    }

    void task_processing_started(core::nanoseconds_t delay) {
        task_latency_.add_time(delay);
    }

    void export_counters(benchmark::State& state) {
        state.counters["t_avg"] = task_latency_.avg();
        state.counters["t_p95"] = task_latency_.p95();
        state.counters["t_count"] = static_cast<double>(task_latency_.count());
    }

private:
    roc::helper::Counter task_latency_;
};

// ============================================================
// Test pipeline implementation (inherits from abstract PipelineLoop)
// ============================================================
class TestPipeline : public PipelineLoop,
                     public IPipelineTaskCompleter,
                     private IPipelineTaskScheduler,
                     public ctl::ControlTaskExecutor<TestPipeline> {
public:
    class Task : public PipelineTask {
    public:
        Task()
            : start_time_(0) {
        }

        void start() {
            start_time_ = core::timestamp(core::ClockMonotonic);
        }

        core::nanoseconds_t elapsed_time() const {
            return core::timestamp(core::ClockMonotonic) - start_time_;
        }

    private:
        core::nanoseconds_t start_time_;
    };

    TestPipeline(const PipelineLoopConfig& config,
                 ctl::ControlTaskQueue& control_queue,
                 BenchStats& stats,
                 core::nanoseconds_t frame_duration,
                 double processing_ratio)
        : PipelineLoop(*this,
                       config,
                       audio::SampleSpec(SampleRate,
                                         audio::PcmSubformat_Raw,
                                         audio::ChanLayout_Surround,
                                         audio::ChanOrder_Smpte,
                                         Chans),
                       frame_pool,
                       frame_buffer_pool_(),
                       Dir_WriteFrames)
        , stats_(stats)
        , control_queue_(control_queue)
        , control_task_(*this)
        , frame_duration_(frame_duration)
        , frame_processing_duration_(
              (core::nanoseconds_t)(frame_duration * processing_ratio)) {
    }

    ~TestPipeline() {
        stop_and_wait();
    }

    void stop_and_wait() {
        control_queue_.async_cancel(control_task_);
        control_queue_.wait(control_task_);

        while (num_pending_tasks() != 0) {
            process_tasks();
        }
    }

    void export_counters(benchmark::State& state) {
        PipelineLoop::Stats st = stats_ref();

        if (st.task_processed_total > 0) {
            state.counters["tp_plc"] =
                roc::helper::round_digits(double(st.task_processed_in_place) / double(st.task_processed_total),
                             3);
            state.counters["tp_frm"] =
                roc::helper::round_digits(double(st.task_processed_in_frame) / double(st.task_processed_total),
                             3);
        }

        state.counters["pr"] = static_cast<double>(st.preemptions);
        state.counters["ss"] = static_cast<double>(st.scheduler_calls);
        state.counters["sc"] = static_cast<double>(st.scheduler_cancellations);
    }

    virtual void pipeline_task_completed(PipelineTask& basic_task) {
        Task& task = (Task&)basic_task;
        delete &task;
    }

    core::nanoseconds_t frame_duration() const {
        return frame_duration_;
    }

    size_t frame_sample_count() const {
        return (size_t)(frame_duration_ / core::Microsecond);
    }

    using PipelineLoop::process_subframes_and_tasks;

private:
    // Create frame buffer pool with appropriate size
    static core::SlabPool<core::Buffer>& frame_buffer_pool_() {
        // Max frame size for 20ms at 1MHz sample rate
        static const size_t MaxFrameByteCount = 20000 * sizeof(audio::sample_t);
        static core::SlabPool<core::Buffer> pool(
            "frame_buffer_pool", arena, sizeof(core::Buffer) + MaxFrameByteCount);
        return pool;
    }

    struct BackgroundProcessingTask : ctl::ControlTask {
        BackgroundProcessingTask(TestPipeline& p)
            : ControlTask(&TestPipeline::do_processing_)
            , pipeline(p) {
        }

        TestPipeline& pipeline;
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
        // Simulate frame processing with busy-wait (NOT sleep!)
        roc::helper::busy_wait(frame_processing_duration_);
        return status::StatusOK;
    }

    virtual bool process_task_imp(PipelineTask& basic_task) {
        Task& task = (Task&)basic_task;
        stats_.task_processing_started(task.elapsed_time());
        roc::helper::busy_wait(TaskProcessingDuration);
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

    BenchStats& stats_;
    ctl::ControlTaskQueue& control_queue_;
    BackgroundProcessingTask control_task_;
    core::nanoseconds_t frame_duration_;
    core::nanoseconds_t frame_processing_duration_;
};

// ============================================================
// Task scheduling thread
// ============================================================
class TaskThread : public core::Thread {
public:
    TaskThread(TestPipeline& pipeline, size_t num_tasks)
        : pipeline_(pipeline)
        , num_tasks_(num_tasks)
        , tasks_scheduled_(0)
        , stop_(false) {
    }

    void stop() {
        stop_ = true;
    }

    size_t tasks_scheduled() const {
        return tasks_scheduled_;
    }

private:
    virtual void run() {
        for (size_t i = 0; i < num_tasks_ && !stop_; i++) {
            TestPipeline::Task* task = new TestPipeline::Task;
            task->start();
            pipeline_.schedule(*task, pipeline_);
            tasks_scheduled_++;
        }
    }

    TestPipeline& pipeline_;
    size_t num_tasks_;
    size_t tasks_scheduled_;
    core::AtomicBool stop_;
};

// ============================================================
// Frame writer thread (simulates audio callback)
// ============================================================
class FrameWriter {
public:
    FrameWriter(TestPipeline& pipeline, benchmark::State& state)
        : pipeline_(pipeline)
        , state_(state) {
    }

    void run() {
        core::Ticker ticker(SampleRate);

        size_t ts = 0;
        const size_t frame_sample_count = pipeline_.frame_sample_count();
        const size_t frame_byte_count = frame_sample_count * sizeof(audio::sample_t);

        audio::FramePtr frame = frame_factory_().allocate_frame(frame_byte_count);
        frame->set_raw(true);
        frame->set_duration((packet::stream_timestamp_t)frame_sample_count);

        while (state_.KeepRunning()) {
            ticker.wait(ts);

            (void)pipeline_.process_subframes_and_tasks(
                *frame, (packet::stream_timestamp_t)frame_sample_count, audio::ModeHard);

            ts += frame->num_raw_samples();
        }
    }

private:
    static audio::FrameFactory& frame_factory_() {
        static const size_t MaxFrameByteCount = 20000 * sizeof(audio::sample_t);
        static core::SlabPool<core::Buffer> buffer_pool(
            "frame_buffer_pool", arena, sizeof(core::Buffer) + MaxFrameByteCount);
        static audio::FrameFactory factory(frame_pool, buffer_pool);
        return factory;
    }

    TestPipeline& pipeline_;
    benchmark::State& state_;
};

// ============================================================
// Benchmark parameters
// ============================================================
struct BenchParams {
    core::nanoseconds_t frame_duration;
    double processing_ratio;
    const char* name;
};

static const BenchParams bench_params[] = {
    // Small frames (1ms)
    {FrameDuration_1ms, ProcessingRatio_Light, "1ms_5pct"},
    {FrameDuration_1ms, ProcessingRatio_Heavy, "1ms_80pct"},

    // Medium frames (5ms)
    {FrameDuration_5ms, ProcessingRatio_Light, "5ms_5pct"},
    {FrameDuration_5ms, ProcessingRatio_Heavy, "5ms_80pct"},

    // Large frames (20ms)
    {FrameDuration_20ms, ProcessingRatio_Light, "20ms_5pct"},
    {FrameDuration_20ms, ProcessingRatio_Heavy, "20ms_80pct"},
};

// ============================================================
// Benchmark: Task Throughput
// Measures how many tasks per second can be processed
// ============================================================
void BM_PipelineLoop_TaskThroughput(benchmark::State& state) {
    const size_t param_idx = (size_t)state.range(0);
    const size_t num_tasks = (size_t)state.range(1);

    if (param_idx >= sizeof(bench_params) / sizeof(bench_params[0])) {
        state.SkipWithError("Invalid param index");
        return;
    }

    const BenchParams& params = bench_params[param_idx];

    ctl::ControlTaskQueue control_queue;
    BenchStats stats;

    PipelineLoopConfig config;
    config.enable_precise_task_scheduling = true;

    TestPipeline pipeline(config, control_queue, stats, params.frame_duration,
                          params.processing_ratio);

    TaskThread task_thr(pipeline, num_tasks);
    FrameWriter frame_wr(pipeline, state);

    (void)task_thr.start();

    frame_wr.run();

    task_thr.stop();
    task_thr.join();

    stats.export_counters(state);
    pipeline.export_counters(state);

    state.counters["tasks_scheduled"] = static_cast<double>(task_thr.tasks_scheduled());
    state.SetLabel(params.name);
}

BENCHMARK(BM_PipelineLoop_TaskThroughput)
    ->Args({0, 1000}) // 1ms, 5%
    ->Args({1, 1000}) // 1ms, 80%
    ->Args({2, 1000}) // 5ms, 5%
    ->Args({3, 1000}) // 5ms, 80%
    ->Args({4, 1000}) // 20ms, 5%
    ->Args({5, 1000}) // 20ms, 80%
    ->Iterations(NumIterations)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================
// Benchmark: Task Latency p95
// Measures the 95th percentile of task scheduling delay
// ============================================================
void BM_PipelineLoop_TaskLatency(benchmark::State& state) {
    const size_t param_idx = (size_t)state.range(0);
    const size_t num_tasks = (size_t)state.range(1);

    if (param_idx >= sizeof(bench_params) / sizeof(bench_params[0])) {
        state.SkipWithError("Invalid param index");
        return;
    }

    const BenchParams& params = bench_params[param_idx];

    ctl::ControlTaskQueue control_queue;
    BenchStats stats;

    PipelineLoopConfig config;
    config.enable_precise_task_scheduling = true;

    TestPipeline pipeline(config, control_queue, stats, params.frame_duration,
                          params.processing_ratio);

    TaskThread task_thr(pipeline, num_tasks);
    FrameWriter frame_wr(pipeline, state);

    (void)task_thr.start();

    frame_wr.run();

    task_thr.stop();
    task_thr.join();

    stats.export_counters(state);
    pipeline.export_counters(state);

    state.SetLabel(params.name);
}

BENCHMARK(BM_PipelineLoop_TaskLatency)
    ->Args({0, 10000}) // 1ms, 5%
    ->Args({1, 10000}) // 1ms, 80%
    ->Args({2, 10000}) // 5ms, 5%
    ->Args({3, 10000}) // 5ms, 80%
    ->Args({4, 10000}) // 20ms, 5%
    ->Args({5, 10000}) // 20ms, 80%
    ->Iterations(NumIterations)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

} // namespace
} // namespace pipeline
} // namespace roc
