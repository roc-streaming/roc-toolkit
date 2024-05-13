/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <benchmark/benchmark.h>

#include "roc_core/mpsc_queue.h"
#include "roc_core/mutex.h"
#include "roc_core/thread.h"

namespace roc {
namespace core {
namespace {

enum { BatchSize = 10000, NumIterations = 5000000, NumThreads = 16 };

#if defined(ROC_BENCHMARK_USE_ACCESSORS)
inline int get_thread_index(const benchmark::State& state) {
    return state.thread_index();
}
#else
inline int get_thread_index(const benchmark::State& state) {
    return state.thread_index;
}
#endif

struct Object : MpscQueueNode<> {};

class BM_MpscQueue : public benchmark::Fixture {
public:
    BM_MpscQueue() {
        for (int n = 0; n < NumThreads; n++) {
            objs_[n] = NULL;
            n_obj_[n] = 0;
        }
    }

    inline MpscQueue<Object, NoOwnership>& get_queue() {
        return queue_;
    }

    inline Object& alloc_object(int thread_index) {
        return objs_[thread_index][n_obj_[thread_index]++];
    }

    virtual void SetUp(const benchmark::State&) {
        Mutex::Lock lock(mutex_);

        for (int n = 0; n < NumThreads; n++) {
            if (!objs_[n]) {
                objs_[n] = new Object[NumIterations];
                n_obj_[n] = 0;
            }
        }
    }

    virtual void TearDown(const benchmark::State&) {
        Mutex::Lock lock(mutex_);

        while (queue_.pop_front_exclusive()) {
        }

        for (int n = 0; n < NumThreads; n++) {
            roc_panic_if(n_obj_[n] > NumIterations);

            delete[] objs_[n];
            objs_[n] = NULL;
        }
    }

private:
    MpscQueue<Object, NoOwnership> queue_;

    Object* objs_[NumThreads];
    size_t n_obj_[NumThreads];

    Mutex mutex_;
};

class PushThread : public core::Thread {
public:
    PushThread()
        : bf_(NULL)
        , thread_index_(0) {
    }

    void init(BM_MpscQueue& bf, int thread_index) {
        bf_ = &bf;
        thread_index_ = thread_index;
    }

private:
    virtual void run() {
        MpscQueue<Object, NoOwnership>& queue = bf_->get_queue();

        for (int i = 0; i < NumIterations; i++) {
            queue.push_back(bf_->alloc_object(thread_index_));
        }
    }

    BM_MpscQueue* bf_;
    int thread_index_;
};

BENCHMARK_DEFINE_F(BM_MpscQueue, PushBack)(benchmark::State& state) {
    MpscQueue<Object, NoOwnership>& queue = get_queue();

    while (state.KeepRunningBatch(BatchSize)) {
        for (int n = 0; n < BatchSize; n++) {
            queue.push_back(alloc_object(get_thread_index(state)));
        }
    }
}

BENCHMARK_REGISTER_F(BM_MpscQueue, PushBack)
    ->ThreadRange(1, NumThreads)
    ->Iterations(NumIterations)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_DEFINE_F(BM_MpscQueue, TryPopFront)(benchmark::State& state) {
    const int64_t num_push_threads_arg = state.range(0);

    PushThread* push_threads = new PushThread[size_t(num_push_threads_arg)];

    for (int n = 0; n < num_push_threads_arg; n++) {
        push_threads[n].init(*this, n);
        (void)push_threads[n].start();
    }

    MpscQueue<Object, NoOwnership>& queue = get_queue();

    while (state.KeepRunningBatch(BatchSize)) {
        for (int n = 0; n < BatchSize; n++) {
            queue.try_pop_front_exclusive();
        }
    }

    for (int n = 0; n < num_push_threads_arg; n++) {
        push_threads[n].join();
    }

    delete[] push_threads;
}

BENCHMARK_REGISTER_F(BM_MpscQueue, TryPopFront)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Iterations(NumIterations)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_DEFINE_F(BM_MpscQueue, PopFront)(benchmark::State& state) {
    const int64_t num_push_threads_arg = state.range(0);

    PushThread* push_threads = new PushThread[size_t(num_push_threads_arg)];

    for (int n = 0; n < num_push_threads_arg; n++) {
        push_threads[n].init(*this, n);
        (void)push_threads[n].start();
    }

    MpscQueue<Object, NoOwnership>& queue = get_queue();

    while (state.KeepRunningBatch(BatchSize)) {
        for (int n = 0; n < BatchSize; n++) {
            queue.pop_front_exclusive();
        }
    }

    for (int n = 0; n < num_push_threads_arg; n++) {
        push_threads[n].join();
    }

    delete[] push_threads;
}

BENCHMARK_REGISTER_F(BM_MpscQueue, PopFront)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Arg(16)
    ->Iterations(NumIterations)
    ->Unit(benchmark::kMicrosecond);

} // namespace
} // namespace core
} // namespace roc
