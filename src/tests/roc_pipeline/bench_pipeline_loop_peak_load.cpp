/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <benchmark/benchmark.h>

#include "roc_core/atomic.h"
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
// This benchmarks emulate the following setup / use case:
//  1. frame length is a few milliseconds
//  2. frame computation is heavy and is also a few milliseconds
//  3. task computation time is a few microseconds
//  4. there is a load peak currently and tasks are scheduled frequently
//  5. tasks are coming in bursts, a few tasks in a burst each millisecond
//
// This is not the most common use case. Usually, pipeline tasks are scheduled
// relatively rarely. However, we want to test this specific case to ensure
// that a load peak leading to scheduling many tasks wont hurt frame processing
// timings.
//
// ----------
// Benchmarks
// ----------
//
// Bench_NoTasks         - frames without tasks
// Bench_NoPreciseSched  - frames and tasks, precise task scheduling is disabled
// Bench_Normal          - frames and tasks, precise task scheduling is enabled
//
// The first benchmark gives us an idea how the unloaded pipeline operates and
// what are its normal frame processing timings.
//
// The second benchmark demonstrates that without precise task scheduling enabled,
// frame processing delays (fb_avg, fb_p95) and thread preemptions (pr) grow
// in a linear direction with the rate of incoming tasks (try to increase MaxTaskBurst
// or decrease MaxTaskDelay).
//
// The third benchmark uses the default pipeline mode, demonstrating that with the
// precise task scheduling enabled:
//  - frame processing delays are independent on the task rate
//  - delay before frame processing (fb_avg, fb_p95) is almost not affected
//  - delay after frame processing (fa_avg, fa_p95) is increased, but in a controllable
//    way, i.e. by config.max_inframe_task_processing plus average duration of one task
//  - thread preemptions (pr) are virtually non-existent, replaced by scheduler
//    cancellations (sc)
//  - task processing time (t_avg t_p95) is slightly increased
//
// --------------
// Output columns
// --------------
//
// (all time units are microseconds)
//
// Time        -  one frame wall clock time
// CPU         -  one frame CPU time
// Iterations  -  number of frames
//
// fb_avg      -  average delay between process_subframes_and_tasks() and
//                process_subframe_imp() calls (i.e. delay before frame processing)
// fb_p95      -  95% percentile of the above
//
// fa_avg      -  average delay between return from process_subframe_imp() and return from
//                process_subframes_and_tasks() (i.e. delay after frame processing)
// fa_p95      -  95% percentile of the above
//
// t_avg       -  average delay between schedule() and process_task_imp() calls
//                (i.e. task processing delay)
// t_p95       -  95% percentile of the above
//
// tp_frm      -  percentage (0..1) of tasks processed within
//                process_subframes_and_tasks() call
// tp_plc      -  percentage (0..1) of tasks processed in-place within schedule() call
//
// pr          -  number of times when schedule() or process_tasks() was preempted by
//                concurrent process_subframes_and_tasks() call
//
// ss          -  number of time when schedule_task_processing() was called
// sc          -  number of time when cancek_task_processing() was called

enum {
    SampleRate = 1000000, // 1 sample = 1 us (for convenience)
    Chans = 0x1,
    FrameSize = 5000, // duration of the frame (5000 = 5ms)
    NumIterations = 3000,
    WarmupIterations = 10,
    FrameBufSize = 100
};

// computation time of a frame
const core::nanoseconds_t FrameProcessingDuration = 3 * core::Millisecond;

// computation time of a task
const core::nanoseconds_t MinTaskProcessingDuration = 5 * core::Microsecond;
const core::nanoseconds_t MaxTaskProcessingDuration = 15 * core::Microsecond;

// delay between enqueueing task bursts
const core::nanoseconds_t MinTaskDelay = 0;
const core::nanoseconds_t MaxTaskDelay = core::Millisecond;

// number of tasks in burst
const size_t MinTaskBurst = 1;
const size_t MaxTaskBurst = 10;

core::HeapArena arena;

core::SlabPool<audio::Frame> frame_pool("frame_pool", arena);
core::SlabPool<core::Buffer> frame_buffer_pool("frame_buffer_pool", arena, FrameBufSize);

audio::FrameFactory frame_factory(frame_pool, frame_buffer_pool);

double round_digits(double x, unsigned int digits) {
    double fac = pow(10, digits);
    return round(x * fac) / fac;
}

void busy_wait(core::nanoseconds_t delay) {
    const core::nanoseconds_t deadline = core::timestamp(core::ClockMonotonic) + delay;
    for (;;) {
        if (core::timestamp(core::ClockMonotonic) >= deadline) {
            return;
        }
    }
}

class Counter {
private:
    enum { NumBuckets = 500 };

public:
    Counter()
        : last_(0)
        , total_(0)
        , count_(0)
        , warmed_up_(false) {
        memset(buckets_, 0, sizeof(buckets_));
    }

    void begin() {
        last_ = core::timestamp(core::ClockMonotonic);
    }

    void end() {
        add_time(core::timestamp(core::ClockMonotonic) - last_);
    }

    void add_time(core::nanoseconds_t t) {
        if (count_ == WarmupIterations && !warmed_up_) {
            *this = Counter();
            warmed_up_ = true;
        }

        total_ += t;
        count_++;

        for (int n = NumBuckets; n > 0; n--) {
            if (t <= core::Microsecond * 10 * (n + 1)) {
                buckets_[n]++;
            } else {
                break;
            }
        }
    }

    double avg() const {
        return round_digits(double(total_) / count_ / 1000, 3);
    }

    double p95() const {
        for (int n = 0; n < NumBuckets; n++) {
            const double ratio = double(buckets_[n]) / count_;
            if (ratio >= 0.95) {
                return 10 * (n + 1);
            }
        }
        return 1. / 0.;
    }

private:
    core::nanoseconds_t last_;

    core::nanoseconds_t total_;
    size_t count_;

    core::nanoseconds_t buckets_[NumBuckets];

    bool warmed_up_;
};

class DelayStats {
public:
    void reset() {
        task_processing_delay_ = Counter();
        frame_delay_before_processing_ = Counter();
        frame_delay_after_processing_ = Counter();
    }

    void task_processing_started(core::nanoseconds_t t) {
        task_processing_delay_.add_time(t);
    }

    void frame_started() {
        frame_delay_before_processing_.begin();
    }

    void frame_processing_started() {
        frame_delay_before_processing_.end();
    }

    void frame_processing_finished() {
        frame_delay_after_processing_.begin();
    }

    void frame_finished() {
        frame_delay_after_processing_.end();
    }

    void export_counters(benchmark::State& state) {
        state.counters["t_avg"] = task_processing_delay_.avg();
        state.counters["t_p95"] = task_processing_delay_.p95();

        state.counters["fb_avg"] = frame_delay_before_processing_.avg();
        state.counters["fb_p95"] = frame_delay_before_processing_.p95();

        state.counters["fa_avg"] = frame_delay_after_processing_.avg();
        state.counters["fa_p95"] = frame_delay_after_processing_.p95();
    }

private:
    Counter task_processing_delay_;
    Counter frame_delay_before_processing_;
    Counter frame_delay_after_processing_;
};

class TestPipeline : public PipelineLoop,
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
                 DelayStats& stats)
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
        , stats_(stats)
        , control_queue_(control_queue)
        , control_task_(*this) {
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

        state.counters["tp_plc"] =
            round_digits(double(st.task_processed_in_place) / st.task_processed_total, 3);

        state.counters["tp_frm"] =
            round_digits(double(st.task_processed_in_frame) / st.task_processed_total, 3);

        state.counters["pr"] = st.preemptions;

        state.counters["ss"] = st.scheduler_calls;
        state.counters["sc"] = st.scheduler_cancellations;
    }

    using PipelineLoop::process_subframes_and_tasks;

private:
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
        stats_.frame_processing_started();
        busy_wait(FrameProcessingDuration);
        stats_.frame_processing_finished();
        return status::StatusOK;
    }

    virtual bool process_task_imp(PipelineTask& basic_task) {
        Task& task = (Task&)basic_task;
        stats_.task_processing_started(task.elapsed_time());
        busy_wait((core::nanoseconds_t)core::fast_random_range(
            MinTaskProcessingDuration, MaxTaskProcessingDuration));
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

    DelayStats& stats_;

    ctl::ControlTaskQueue& control_queue_;
    BackgroundProcessingTask control_task_;
};

class TaskThread : public core::Thread, private IPipelineTaskCompleter {
public:
    TaskThread(TestPipeline& pipeline)
        : pipeline_(pipeline)
        , stop_(false) {
    }

    void stop() {
        stop_ = true;
    }

private:
    virtual void run() {
        while (!stop_) {
            core::sleep_for(
                core::ClockMonotonic,
                (core::nanoseconds_t)core::fast_random_range(MinTaskDelay, MaxTaskDelay));

            const size_t n_tasks = core::fast_random_range(MinTaskBurst, MaxTaskBurst);

            for (size_t n = 0; n < n_tasks; n++) {
                TestPipeline::Task* task = new TestPipeline::Task;

                task->start();

                pipeline_.schedule(*task, *this);
            }
        }
    }

    virtual void pipeline_task_completed(PipelineTask& basic_task) {
        TestPipeline::Task& task = (TestPipeline::Task&)basic_task;
        delete &task;
    }

    TestPipeline& pipeline_;
    core::Atomic<int> stop_;
};

class FrameWriter {
public:
    FrameWriter(TestPipeline& pipeline, DelayStats& stats, benchmark::State& state)
        : pipeline_(pipeline)
        , stats_(stats)
        , state_(state) {
    }

    void run() {
        core::Ticker ticker(SampleRate);

        size_t ts = 0;

        audio::FramePtr frame = frame_factory.allocate_frame(FrameSize);

        while (state_.KeepRunning()) {
            ticker.wait(ts);

            stats_.frame_started();

            (void)pipeline_.process_subframes_and_tasks(*frame, frame->duration(),
                                                        audio::ModeHard);

            stats_.frame_finished();

            ts += frame->num_raw_samples();
        }
    }

private:
    TestPipeline& pipeline_;
    DelayStats& stats_;
    benchmark::State& state_;
};

void BM_PipelinePeakLoad_NoTasks(benchmark::State& state) {
    ctl::ControlTaskQueue control_queue;

    DelayStats stats;

    PipelineLoopConfig config;
    TestPipeline pipeline(config, control_queue, stats);

    FrameWriter frame_wr(pipeline, stats, state);

    frame_wr.run();

    stats.export_counters(state);
    pipeline.export_counters(state);
}

BENCHMARK(BM_PipelinePeakLoad_NoTasks)
    ->Iterations(NumIterations)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

void BM_PipelinePeakLoad_PreciseSchedOff(benchmark::State& state) {
    ctl::ControlTaskQueue control_queue;

    DelayStats stats;

    PipelineLoopConfig config;
    config.enable_precise_task_scheduling = false;

    TestPipeline pipeline(config, control_queue, stats);

    TaskThread task_thr(pipeline);

    FrameWriter frame_wr(pipeline, stats, state);

    (void)task_thr.start();

    frame_wr.run();

    task_thr.stop();
    task_thr.join();

    stats.export_counters(state);
    pipeline.export_counters(state);
}

BENCHMARK(BM_PipelinePeakLoad_PreciseSchedOff)
    ->Iterations(NumIterations)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

void BM_PipelinePeakLoad_PreciseSchedOn(benchmark::State& state) {
    ctl::ControlTaskQueue control_queue;

    DelayStats stats;

    PipelineLoopConfig config;
    config.enable_precise_task_scheduling = true;

    TestPipeline pipeline(config, control_queue, stats);

    TaskThread task_thr(pipeline);

    FrameWriter frame_wr(pipeline, stats, state);

    (void)task_thr.start();

    frame_wr.run();

    task_thr.stop();
    task_thr.join();

    stats.export_counters(state);
    pipeline.export_counters(state);
}

BENCHMARK(BM_PipelinePeakLoad_PreciseSchedOn)
    ->Iterations(NumIterations)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

} // namespace
} // namespace pipeline
} // namespace roc
