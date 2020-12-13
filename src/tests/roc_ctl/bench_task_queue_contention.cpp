/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <benchmark/benchmark.h>

#include "roc_core/fast_random.h"
#include "roc_ctl/task_queue.h"

namespace roc {
namespace ctl {
namespace {

enum {
    NumScheduleIterations = 2000000,
    NumScheduleAfterIterations = 20000,
    NumThreads = 8,
    BatchSize = 1000,
};

const core::nanoseconds_t MaxDelay = 100 * core::Millisecond;
//const core::nanoseconds_t StartTime = 1000 * core::Second;

class NoopQueue : public TaskQueue {
public:
    class Task : public TaskQueue::Task {
    public:
        Task() {
        }
    };

    ~NoopQueue() {
        stop_and_wait();
    }


    using TaskQueue::stop_and_wait;

private:
    virtual TaskResult process_task_imp(TaskQueue::Task&) {
        return TaskSucceeded;
    }

    // Dummy implementation of TaskQueue::timestamp_imp().
    virtual core::nanoseconds_t timestamp_imp() const {
        return 0;
    }

};

class NoopHandler : public TaskQueue::ICompletionHandler {
public:
    virtual void control_task_finished(TaskQueue::Task&) {
    }

};

struct BM_QueueContention : benchmark::Fixture {
    NoopQueue queue;
    NoopHandler handler;
};

BENCHMARK_DEFINE_F(BM_QueueContention, Schedule)(benchmark::State& state) {
    NoopQueue::Task* tasks = new NoopQueue::Task[NumScheduleIterations];
    size_t n_task = 0;

    while (state.KeepRunningBatch(BatchSize)) {
        for (int n = 0; n < BatchSize; n++) {
            queue.schedule(tasks[n_task++], &handler);
        }
    }

    for (int n = 0; n < NumScheduleIterations; n++) {
        queue.wait(tasks[n]);
    }

    delete[] tasks;
}

BENCHMARK_REGISTER_F(BM_QueueContention, Schedule)
    ->ThreadRange(1, NumThreads)
    ->Iterations(NumScheduleIterations)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_DEFINE_F(BM_QueueContention, ScheduleAt)(benchmark::State& state) {
    NoopQueue::Task* tasks = new NoopQueue::Task[NumScheduleAfterIterations];
    size_t n_task = 0;

    core::nanoseconds_t* delays = new core::nanoseconds_t[NumScheduleAfterIterations];
    for (int n = 0; n < NumScheduleAfterIterations; n++) {
        delays[n] = core::fast_random(0, MaxDelay);
    }

    while (state.KeepRunningBatch(BatchSize)) {
        for (int n = 0; n < BatchSize; n++) {
            queue.schedule_at(tasks[n_task], core::timestamp() + delays[n_task],
                              &handler);
            n_task++;
        }
    }

    for (int n = 0; n < NumScheduleAfterIterations; n++) {
        queue.wait(tasks[n]);
    }

    delete[] tasks;
    delete[] delays;
}

BENCHMARK_REGISTER_F(BM_QueueContention, ScheduleAt)
    ->ThreadRange(1, NumThreads)
    ->Iterations(NumScheduleAfterIterations)
    ->Unit(benchmark::kMicrosecond);

} // namespace
} // namespace ctl
} // namespace roc
