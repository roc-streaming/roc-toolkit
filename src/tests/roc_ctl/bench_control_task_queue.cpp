/*
 * Copyright (c) Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "bench_helper.h"
#include <benchmark/benchmark.h>

#include "roc_core/heap_arena.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_ctl/control_task_executor.h"
#include "roc_ctl/control_task_queue.h"

namespace roc {
namespace ctl {
namespace {

// --------
// Overview
// --------
//
// This benchmark measures ControlTaskQueue performance:
//   1. Task throughput for schedule() - immediate task scheduling
//   2. Task throughput for schedule_at() - delayed task scheduling
//   3. Task latency p95 - 95th percentile of delay between schedule and execution
//
// --------------
// Output columns
// --------------
//
// t_avg       -  average delay between schedule() and task execution
// t_p95       -  95th percentile of the above (returns -1 if >50ms)
// tasks_total -  total number of tasks processed

class TimedTask;

// ============================================================
// Test task executor
// ============================================================
class TestExecutor : public ControlTaskExecutor<TestExecutor> {
public:
    TestExecutor()
        : tasks_processed_(0) {
    }

    size_t tasks_processed() const {
        return tasks_processed_;
    }

    // Defined after TimedTask is complete
    ControlTaskResult process_task(ControlTask& task);

private:
    size_t tasks_processed_;
};

// ============================================================
// Test task with timing (stores start time as member)
// ============================================================
class TimedTask : public ControlTask {
public:
    TimedTask()
        : ControlTask(&TestExecutor::process_task)
        , start_time_(0)
        , stats_(NULL) {
    }

    void setup(roc::helper::Counter* stats) {
        stats_ = stats;
    }

    void start() {
        start_time_ = core::timestamp(core::ClockMonotonic);
    }

    void record_latency() {
        if (stats_ != NULL && start_time_ != 0) {
            core::nanoseconds_t latency =
                core::timestamp(core::ClockMonotonic) - start_time_;
            stats_->add_time(latency);
        }
    }

private:
    core::nanoseconds_t start_time_;
    roc::helper::Counter* stats_;
};

// Definition of TestExecutor::process_task (after TimedTask is complete)
ControlTaskResult TestExecutor::process_task(ControlTask& task) {
    TimedTask& timed_task = static_cast<TimedTask&>(task);
    timed_task.record_latency();
    tasks_processed_++;
    return ControlTaskSuccess;
}

// ============================================================
// Benchmark: schedule() Throughput
// Measures how many immediate tasks per second can be processed
// ============================================================
void BM_ControlTaskQueue_Schedule_Throughput(benchmark::State& state) {
    const size_t num_tasks = static_cast<size_t>(state.range(0));

    roc::helper::Counter stats;
    TestExecutor executor;
    ControlTaskQueue queue;

    // Pre-allocate tasks array
    TimedTask* tasks = new TimedTask[num_tasks];
    for (size_t i = 0; i < num_tasks; i++) {
        tasks[i].setup(&stats);
    }

    for (auto _ : state) {
        // Schedule all tasks
        for (size_t i = 0; i < num_tasks; i++) {
            tasks[i].start();
            queue.schedule(tasks[i], executor, NULL);
        }

        // Wait for all tasks to complete
        for (size_t i = 0; i < num_tasks; i++) {
            queue.wait(tasks[i]);
        }
    }

    // Cleanup
    delete[] tasks;

    state.counters["t_avg"] = stats.avg();
    state.counters["t_p95"] = stats.p95();
    state.counters["tasks_total"] = static_cast<double>(executor.tasks_processed());

    state.counters["tasks/sec"] = benchmark::Counter(
        static_cast<double>(executor.tasks_processed()), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_ControlTaskQueue_Schedule_Throughput)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================
// Benchmark: schedule_at() Throughput
// Measures how many delayed tasks per second can be processed
// ============================================================
void BM_ControlTaskQueue_ScheduleAt_Throughput(benchmark::State& state) {
    const size_t num_tasks = static_cast<size_t>(state.range(0));

    roc::helper::Counter stats;
    TestExecutor executor;
    ControlTaskQueue queue;

    // Pre-allocate tasks array
    TimedTask* tasks = new TimedTask[num_tasks];
    for (size_t i = 0; i < num_tasks; i++) {
        tasks[i].setup(&stats);
    }

    for (auto _ : state) {
        core::nanoseconds_t now = core::timestamp(core::ClockMonotonic);

        // Schedule tasks with spread deadlines to avoid queue congestion
        // Each task is scheduled 1us apart
        for (size_t i = 0; i < num_tasks; i++) {
            tasks[i].start();
            queue.schedule_at(tasks[i], now + (core::nanoseconds_t)i * core::Microsecond,
                              executor, NULL);
        }

        // Wait for all tasks to complete
        for (size_t i = 0; i < num_tasks; i++) {
            queue.wait(tasks[i]);
        }
    }

    // Cleanup
    delete[] tasks;

    state.counters["t_avg"] = stats.avg();
    state.counters["t_p95"] = stats.p95();
    state.counters["tasks_total"] = static_cast<double>(executor.tasks_processed());

    state.counters["tasks/sec"] = benchmark::Counter(
        static_cast<double>(executor.tasks_processed()), benchmark::Counter::kIsRate);
}

BENCHMARK(BM_ControlTaskQueue_ScheduleAt_Throughput)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================
// Benchmark: schedule() Latency p95
// Measures the 95th percentile of task scheduling delay
// ============================================================
void BM_ControlTaskQueue_Schedule_Latency(benchmark::State& state) {
    const size_t num_tasks = static_cast<size_t>(state.range(0));

    roc::helper::Counter stats;
    TestExecutor executor;
    ControlTaskQueue queue;

    // Pre-allocate tasks array
    TimedTask* tasks = new TimedTask[num_tasks];
    for (size_t i = 0; i < num_tasks; i++) {
        tasks[i].setup(&stats);
    }

    for (auto _ : state) {
        // Schedule all tasks one by one, measuring latency
        for (size_t i = 0; i < num_tasks; i++) {
            tasks[i].start();
            queue.schedule(tasks[i], executor, NULL);
        }

        // Wait for all tasks to complete
        for (size_t i = 0; i < num_tasks; i++) {
            queue.wait(tasks[i]);
        }
    }

    // Cleanup
    delete[] tasks;

    state.counters["t_avg"] = stats.avg();
    state.counters["t_p95"] = stats.p95();
    state.counters["tasks_total"] = static_cast<double>(executor.tasks_processed());
}

BENCHMARK(BM_ControlTaskQueue_Schedule_Latency)
    ->Arg(1000)
    ->Arg(10000)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

// ============================================================
// Benchmark: schedule_at() Latency p95
// Measures the 95th percentile of delayed task scheduling
// ============================================================
void BM_ControlTaskQueue_ScheduleAt_Latency(benchmark::State& state) {
    const size_t num_tasks = static_cast<size_t>(state.range(0));

    roc::helper::Counter stats;
    TestExecutor executor;
    ControlTaskQueue queue;

    // Pre-allocate tasks array
    TimedTask* tasks = new TimedTask[num_tasks];
    for (size_t i = 0; i < num_tasks; i++) {
        tasks[i].setup(&stats);
    }

    for (auto _ : state) {
        core::nanoseconds_t now = core::timestamp(core::ClockMonotonic);

        // Schedule tasks with spread deadlines to avoid queue congestion
        for (size_t i = 0; i < num_tasks; i++) {
            tasks[i].start();
            queue.schedule_at(tasks[i], now + (core::nanoseconds_t)i * core::Microsecond,
                              executor, NULL);
        }

        // Wait for all tasks to complete
        for (size_t i = 0; i < num_tasks; i++) {
            queue.wait(tasks[i]);
        }
    }

    // Cleanup
    delete[] tasks;

    state.counters["t_avg"] = stats.avg();
    state.counters["t_p95"] = stats.p95();
    state.counters["tasks_total"] = static_cast<double>(executor.tasks_processed());
}

BENCHMARK(BM_ControlTaskQueue_ScheduleAt_Latency)
    ->Arg(1000)
    ->Arg(10000)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

} // namespace
} // namespace ctl
} // namespace roc
