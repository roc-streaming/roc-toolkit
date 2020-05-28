/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/cond.h"
#include "roc_core/mutex.h"
#include "roc_core/stddefs.h"
#include "roc_core/thread.h"
#include "roc_pipeline/task_pipeline.h"

namespace roc {
namespace pipeline {

namespace {

enum {
    SampleRate = 1000000, // 1 sample = 1 us (for convenience)
    Chans = 0x1,
    MinFrameSize = 4000,
    FrameSize = 5000, // duration of the frame (5000 = 5ms)
    MaxFrameSize = 6000,
    MaxSamples = 20000
};

const core::nanoseconds_t MaxInframeProcessing = 100 * core::Microsecond;
const core::nanoseconds_t NoTaskProcessingGap = 200 * core::Microsecond;

const core::nanoseconds_t StartTime = 10000000 * core::Second;

const core::nanoseconds_t FrameProcessingTime = 50 * core::Microsecond;

const float Epsilon = 1e6f;

class TestPipeline : public TaskPipeline, private ITaskScheduler {
public:
    class Task : public TaskPipeline::Task {
    public:
        Task() {
        }
    };

    TestPipeline(const TaskConfig& config)
        : TaskPipeline(*this, config, SampleRate, Chans)
        , blocked_cond_(mutex_)
        , unblocked_cond_(mutex_)
        , blocked_counter_(0)
        , old_blocked_counter_(0)
        , frame_allow_counter_(999999)
        , task_allow_counter_(999999)
        , time_(StartTime)
        , exp_frame_val_(0)
        , exp_frame_sz_(0)
        , exp_sched_deadline_(-1)
        , n_processed_frames_(0)
        , n_processed_tasks_(0)
        , n_sched_calls_(0)
        , n_sched_cancellations_(0) {
    }

    void set_time(core::nanoseconds_t t) {
        core::Mutex::Lock lock(mutex_);
        time_ = t;
    }

    void block_frames() {
        core::Mutex::Lock lock(mutex_);
        frame_allow_counter_ = 0;
        old_blocked_counter_ = blocked_counter_;
    }

    void unblock_one_frame() {
        core::Mutex::Lock lock(mutex_);
        frame_allow_counter_++;
        old_blocked_counter_ = blocked_counter_;
        unblocked_cond_.signal();
    }

    void unblock_all_frames() {
        core::Mutex::Lock lock(mutex_);
        frame_allow_counter_ = 999999;
        old_blocked_counter_ = blocked_counter_;
        unblocked_cond_.signal();
    }

    void block_tasks() {
        core::Mutex::Lock lock(mutex_);
        task_allow_counter_ = 0;
        old_blocked_counter_ = blocked_counter_;
    }

    void unblock_one_task() {
        core::Mutex::Lock lock(mutex_);
        task_allow_counter_++;
        old_blocked_counter_ = blocked_counter_;
        unblocked_cond_.signal();
    }

    void unblock_all_tasks() {
        core::Mutex::Lock lock(mutex_);
        task_allow_counter_ = 999999;
        old_blocked_counter_ = blocked_counter_;
        unblocked_cond_.signal();
    }

    void wait_blocked() {
        core::Mutex::Lock lock(mutex_);
        while (blocked_counter_ == old_blocked_counter_) {
            blocked_cond_.wait();
        }
        old_blocked_counter_ = blocked_counter_;
    }

    size_t num_processed_frames() const {
        core::Mutex::Lock lock(mutex_);
        return n_processed_frames_;
    }

    size_t num_processed_tasks() const {
        core::Mutex::Lock lock(mutex_);
        UNSIGNED_LONGS_EQUAL(n_processed_tasks_,
                             (size_t)get_stats_ref().task_processed_total);
        return n_processed_tasks_;
    }

    size_t num_tasks_processed_in_sched() const {
        core::Mutex::Lock lock(mutex_);
        return (size_t)get_stats_ref().task_processed_in_place;
    }

    size_t num_tasks_processed_in_frame() const {
        core::Mutex::Lock lock(mutex_);
        return (size_t)get_stats_ref().task_processed_in_frame;
    }

    size_t num_tasks_processed_in_proc() const {
        core::Mutex::Lock lock(mutex_);
        return size_t(get_stats_ref().task_processed_total
                      - get_stats_ref().task_processed_in_frame
                      - get_stats_ref().task_processed_in_place);
    }

    size_t num_preemptions() const {
        core::Mutex::Lock lock(mutex_);
        return (size_t)get_stats_ref().preemptions;
    }

    size_t num_sched_calls() const {
        core::Mutex::Lock lock(mutex_);
        UNSIGNED_LONGS_EQUAL(n_sched_calls_, (size_t)get_stats_ref().scheduler_calls);
        return n_sched_calls_;
    }

    size_t num_sched_cancellations() const {
        core::Mutex::Lock lock(mutex_);
        UNSIGNED_LONGS_EQUAL(n_sched_cancellations_,
                             (size_t)get_stats_ref().scheduler_cancellations);
        return n_sched_cancellations_;
    }

    void expect_frame(audio::sample_t val, size_t sz) {
        core::Mutex::Lock lock(mutex_);
        exp_frame_val_ = val;
        exp_frame_sz_ = sz;
    }

    void expect_sched_deadline(core::nanoseconds_t d) {
        core::Mutex::Lock lock(mutex_);
        exp_sched_deadline_ = d;
    }

    using TaskPipeline::num_pending_frames;
    using TaskPipeline::num_pending_tasks;
    using TaskPipeline::process_frame_and_tasks;

private:
    virtual core::nanoseconds_t timestamp_imp() const {
        core::Mutex::Lock lock(mutex_);
        return time_;
    }

    virtual bool process_frame_imp(audio::Frame& frame) {
        core::Mutex::Lock lock(mutex_);
        bool first_iter = true;
        while (frame_allow_counter_ == 0) {
            if (first_iter) {
                blocked_counter_++;
                first_iter = false;
            }
            blocked_cond_.signal();
            unblocked_cond_.wait();
        }
        frame_allow_counter_--;
        roc_panic_if(frame.size() != exp_frame_sz_);
        for (size_t n = 0; n < exp_frame_sz_; n++) {
            roc_panic_if(std::abs(frame.data()[n] - exp_frame_val_) > Epsilon);
        }
        n_processed_frames_++;
        return true;
    }

    virtual bool process_task_imp(TaskPipeline::Task&) {
        core::Mutex::Lock lock(mutex_);
        bool first_iter = true;
        while (task_allow_counter_ == 0) {
            if (first_iter) {
                blocked_counter_++;
                first_iter = false;
            }
            blocked_cond_.signal();
            unblocked_cond_.wait();
        }
        task_allow_counter_--;
        n_processed_tasks_++;
        return true;
    }

    virtual void schedule_task_processing(TaskPipeline& pipeline,
                                          core::nanoseconds_t deadline) {
        core::Mutex::Lock lock(mutex_);
        roc_panic_if(&pipeline != this);
        core::nanoseconds_t expected_deadline = exp_sched_deadline_;
        if (expected_deadline == time_) {
            expected_deadline = 0;
        }
        if (deadline != expected_deadline) {
            roc_panic("unexpected delay:"
                      " time=%llu expected_deadline=%llu actual_deadline=%llu",
                      (unsigned long long)time_, (unsigned long long)expected_deadline,
                      (unsigned long long)deadline);
        }
        n_sched_calls_++;
    }

    virtual void cancel_task_processing(TaskPipeline& pipeline) {
        core::Mutex::Lock lock(mutex_);
        roc_panic_if(&pipeline != this);
        n_sched_cancellations_++;
    }

    core::Mutex mutex_;
    core::Cond blocked_cond_;
    core::Cond unblocked_cond_;

    int blocked_counter_;
    int old_blocked_counter_;

    int frame_allow_counter_;
    int task_allow_counter_;

    core::nanoseconds_t time_;

    audio::sample_t exp_frame_val_;
    size_t exp_frame_sz_;

    core::nanoseconds_t exp_sched_deadline_;

    size_t n_processed_frames_;
    size_t n_processed_tasks_;

    size_t n_sched_calls_;
    size_t n_sched_cancellations_;
};

class TestHandler : public TaskPipeline::ICompletionHandler {
public:
    TestHandler(TestPipeline& pipeline)
        : pipeline_(pipeline)
        , cond_(mutex_)
        , task_(NULL)
        , next_task_(NULL) {
    }

    ~TestHandler() {
        roc_panic_if(task_);
        roc_panic_if(next_task_);
    }

    virtual void pipeline_task_finished(TaskPipeline::Task& task) {
        TaskPipeline::Task* next_task = NULL;

        {
            core::Mutex::Lock lock(mutex_);
            roc_panic_if_not(task.success());
            task_ = &task;
            if (next_task_) {
                next_task = next_task_;
                next_task_ = NULL;
            }
            cond_.broadcast();
        }

        if (next_task) {
            pipeline_.schedule(*next_task, *this);
        }
    }

    TestPipeline::Task* get_task() {
        core::Mutex::Lock lock(mutex_);
        TestPipeline::Task* ret = (TestPipeline::Task*)task_;
        task_ = NULL;
        return ret;
    }

    TestPipeline::Task* wait_task() {
        core::Mutex::Lock lock(mutex_);
        while (!task_) {
            cond_.wait();
        }
        TestPipeline::Task* ret = (TestPipeline::Task*)task_;
        task_ = NULL;
        return ret;
    }

    void set_next_task(TestPipeline::Task& task) {
        core::Mutex::Lock lock(mutex_);
        next_task_ = &task;
    }

private:
    TestPipeline& pipeline_;

    core::Mutex mutex_;
    core::Cond cond_;

    TaskPipeline::Task* task_;
    TaskPipeline::Task* next_task_;
};

class AsyncTaskScheduler : public core::Thread {
public:
    AsyncTaskScheduler(TestPipeline& pipeline,
                       TestPipeline::Task& task,
                       TestHandler* handler)
        : pipeline_(pipeline)
        , task_(task)
        , handler_(handler) {
    }

private:
    virtual void run() {
        if (handler_) {
            pipeline_.schedule(task_, *handler_);
        } else {
            pipeline_.schedule_and_wait(task_);
        }
    }

    TestPipeline& pipeline_;
    TestPipeline::Task& task_;
    TestHandler* handler_;
};

class AsyncTaskProcessor : public core::Thread {
public:
    AsyncTaskProcessor(TestPipeline& pipeline)
        : pipeline_(pipeline) {
    }

private:
    virtual void run() {
        pipeline_.process_tasks();
    }

    TestPipeline& pipeline_;
};

class AsyncFrameWriter : public core::Thread {
public:
    AsyncFrameWriter(TestPipeline& pipeline, audio::Frame& frame)
        : pipeline_(pipeline)
        , frame_(frame) {
    }

private:
    virtual void run() {
        pipeline_.process_frame_and_tasks(frame_);
    }

    TestPipeline& pipeline_;
    audio::Frame& frame_;
};

} // namespace

TEST_GROUP(task_pipeline) {
    audio::sample_t samples[MaxSamples];

    TaskConfig config;

    void setup() {
        config.enable_precise_task_scheduling = true;
        config.min_frame_length_between_tasks = MinFrameSize * core::Microsecond;
        config.max_frame_length_between_tasks = MaxFrameSize * core::Microsecond;
        config.max_inframe_task_processing = MaxInframeProcessing;
        config.task_processing_prohibited_interval = NoTaskProcessingGap;
    }

    void fill_frame(audio::Frame & frame, float val, size_t from, size_t to) {
        CHECK(from <= frame.size());
        CHECK(to <= frame.size());
        for (size_t n = from; n < to; n++) {
            frame.data()[n] = val;
        }
    }
};

TEST(task_pipeline, schedule_and_wait_right_after_creation) {
    TestPipeline pipeline(config);

    TestPipeline::Task task;

    CHECK(!task.success());

    // schedule_and_wait() should process task in-place
    pipeline.schedule_and_wait(task);

    CHECK(task.success());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());
}

TEST(task_pipeline, schedule_right_after_creation) {
    TestPipeline pipeline(config);

    TestHandler handler(pipeline);
    TestPipeline::Task task;

    CHECK(!task.success());

    // schedule() should process task in-place
    pipeline.schedule(task, handler);

    CHECK(task.success());

    CHECK(handler.get_task() == &task);

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());
}

TEST(task_pipeline, schedule_when_can_process_tasks) {
    TestPipeline pipeline(config);

    audio::Frame frame(samples, FrameSize);
    fill_frame(frame, 0.1f, 0, FrameSize);
    pipeline.expect_frame(0.1f, FrameSize);

    pipeline.set_time(StartTime);

    // process_frame_and_tasks() should allow task processing
    // until (StartTime + FrameSize * core::Microsecond - NoTaskProcessingGap / 2)
    CHECK(pipeline.process_frame_and_tasks(frame));

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_frames());

    TestHandler handler(pipeline);
    TestPipeline::Task task;

    // deadline not expired yet (because of "-1")
    pipeline.set_time(StartTime + FrameSize * core::Microsecond - NoTaskProcessingGap / 2
                      - 1);

    // schedule() should process task in-place
    pipeline.schedule(task, handler);

    POINTERS_EQUAL(&task, handler.get_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());
}

TEST(task_pipeline, schedule_when_cant_process_tasks_then_process_frame) {
    TestPipeline pipeline(config);

    audio::Frame frame1(samples, FrameSize);
    fill_frame(frame1, 0.1f, 0, FrameSize);
    pipeline.expect_frame(0.1f, FrameSize);

    pipeline.set_time(StartTime);

    // process_frame_and_tasks() should allow task processing
    // until (StartTime + FrameSize * core::Microsecond - NoTaskProcessingGap / 2)
    CHECK(pipeline.process_frame_and_tasks(frame1));

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_frames());

    TestHandler handler(pipeline);
    TestPipeline::Task task;

    // deadline expired
    pipeline.set_time(StartTime + FrameSize * core::Microsecond
                      - NoTaskProcessingGap / 2);

    // this deadline will be passed to schedule_task_processing()
    // if this deadline expires, it means that process_frame_and_tasks() was not
    // called for some reason and didn't process our tasks, so we should call
    // process_tasks()
    pipeline.expect_sched_deadline(StartTime + FrameSize * core::Microsecond
                                   + NoTaskProcessingGap / 2);

    // schedule() should see that deadline expired and add this task to the queue and
    // call schedule_task_processing() to process tasks later
    pipeline.schedule(task, handler);

    POINTERS_EQUAL(NULL, handler.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    audio::Frame frame2(samples, FrameSize);
    fill_frame(frame2, 0.2f, 0, FrameSize);
    pipeline.expect_frame(0.2f, FrameSize);

    pipeline.set_time(StartTime + FrameSize * core::Microsecond);

    // process_frame_and_tasks() should call cancel_task_processing() and
    // process the task from the queue
    CHECK(pipeline.process_frame_and_tasks(frame2));

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_processed_frames());

    POINTERS_EQUAL(&task, handler.get_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());
}

TEST(task_pipeline, schedule_when_cant_process_tasks_then_process_tasks) {
    TestPipeline pipeline(config);

    audio::Frame frame1(samples, FrameSize);
    fill_frame(frame1, 0.1f, 0, FrameSize);
    pipeline.expect_frame(0.1f, FrameSize);

    pipeline.set_time(StartTime);

    // process_frame_and_tasks() should allow task processing
    // until (StartTime + FrameSize * core::Microsecond - NoTaskProcessingGap / 2)
    CHECK(pipeline.process_frame_and_tasks(frame1));

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_frames());

    TestHandler handler(pipeline);
    TestPipeline::Task task;

    // current frame deadline expired
    pipeline.set_time(StartTime + FrameSize * core::Microsecond
                      - NoTaskProcessingGap / 2);

    // this deadline will be passed to schedule_task_processing()
    // if this deadline expires, it means that process_frame_and_tasks() was not
    // called for some reason and didn't process our tasks, so we should call
    // process_tasks()
    pipeline.expect_sched_deadline(StartTime + FrameSize * core::Microsecond
                                   + NoTaskProcessingGap / 2);

    // schedule() should see that deadline expired and add this task to the queue and
    // call schedule_task_processing() to process tasks later
    pipeline.schedule(task, handler);

    POINTERS_EQUAL(NULL, handler.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // next frame deadline not expired yet (because of "-1")
    pipeline.set_time(StartTime + FrameSize * core::Microsecond + NoTaskProcessingGap / 2
                      - 1);

    // will not process any tasks because deadline not expired yet
    // and we're still waiting for process_frame_and_tasks() call
    pipeline.process_tasks();

    POINTERS_EQUAL(NULL, handler.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // next frame deadline expired
    pipeline.set_time(StartTime + FrameSize * core::Microsecond
                      + NoTaskProcessingGap / 2);

    // process_frame_and_tasks() was not called before next frame deadline
    // we start processing tasks again
    // process_tasks() should process our task
    pipeline.process_tasks();

    POINTERS_EQUAL(&task, handler.get_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());
}

TEST(task_pipeline, schedule_when_another_schedule_is_running_then_process_tasks) {
    TestPipeline pipeline(config);
    TestHandler handler(pipeline);

    pipeline.set_time(StartTime);

    // next process_task_imp() call will block
    pipeline.block_tasks();

    TestPipeline::Task task1;

    // AsyncTaskScheduler will call schedule() from another thread
    // it will call process_task_imp() and block
    AsyncTaskScheduler ts(pipeline, task1, &handler);
    ts.start();

    // wait until background schedule() is blocked
    pipeline.wait_blocked();

    POINTERS_EQUAL(NULL, handler.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    TestPipeline::Task task2;

    // this schedule() should see that the pipeline is busy (because it's
    // locked by process_task_imp()), add task to queue, and return
    pipeline.schedule(task2, handler);

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // this deadline will be passed to schedule_task_processing()
    // in this case it means "process tasks immediately"
    pipeline.expect_sched_deadline(StartTime);

    // unblock blocked process_task_imp()
    pipeline.unblock_all_tasks();

    // wait until background schedule() finishes
    // it should process the first task, see that a new task was added,
    // call schedule_processing_tasks(), and return
    ts.join();

    POINTERS_EQUAL(&task1, handler.wait_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // process_tasks() should process the second task that is still in queue
    pipeline.process_tasks();

    POINTERS_EQUAL(&task2, handler.wait_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());
}

TEST(task_pipeline, schedule_when_process_tasks_is_running) {
    TestPipeline pipeline(config);

    pipeline.set_time(StartTime);

    // next process_task_imp() call will block
    pipeline.block_tasks();

    TestHandler handler1(pipeline);
    TestPipeline::Task task1;

    // AsyncTaskScheduler will call schedule() from another thread
    // it will call process_task_imp() and block
    AsyncTaskScheduler ts(pipeline, task1, &handler1);
    ts.start();

    // wait until background schedule() is blocked
    pipeline.wait_blocked();

    POINTERS_EQUAL(NULL, handler1.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    TestHandler handler2(pipeline);
    TestPipeline::Task task2;

    // this schedule() should see that the pipeline is busy (because it's
    // locked by process_task_imp()), add task to queue, and return
    pipeline.schedule(task2, handler2);

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // this deadline will be passed to schedule_task_processing()
    // in this case it means "process tasks immediately"
    pipeline.expect_sched_deadline(StartTime);

    // unblock blocked process_task_imp()
    pipeline.unblock_one_task();

    // wait until background schedule() finishes
    // it should process the first task, see that a new task was added,
    // call schedule_processing_tasks(), and return
    ts.join();

    POINTERS_EQUAL(&task1, handler1.wait_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // next process_task_imp() call will block (again)
    pipeline.block_tasks();

    // AsyncTaskProcessor will call process_tasks() from another thread
    // it will call process_task_imp() and block
    AsyncTaskProcessor tp(pipeline);
    tp.start();

    // wait until background process_tasks() is blocked
    pipeline.wait_blocked();

    TestHandler handler3(pipeline);
    TestPipeline::Task task3;

    // this schedule() should see that the pipeline is busy (because it's
    // locked by process_task_imp()), add task to queue, and return
    pipeline.schedule(task3, handler3);

    POINTERS_EQUAL(NULL, handler2.get_task());
    POINTERS_EQUAL(NULL, handler3.get_task());

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // unblock blocked process_task_imp()
    pipeline.unblock_all_tasks();

    // wait until background process_tasks() finishes
    // it should process the first task, see that a new task was added,
    // and process it as well
    tp.join();

    POINTERS_EQUAL(&task2, handler2.get_task());
    POINTERS_EQUAL(&task3, handler3.get_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(3, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());
}

TEST(task_pipeline, schedule_when_processing_frame) {
    TestPipeline pipeline(config);

    audio::Frame frame(samples, FrameSize);
    fill_frame(frame, 0.1f, 0, FrameSize);
    pipeline.expect_frame(0.1f, FrameSize);

    // next process_frame_imp() call will block
    pipeline.block_frames();

    // AsyncFrameWriter will call process_frame_and_tasks() from background thread
    AsyncFrameWriter fw(pipeline, frame);
    fw.start();

    // wait until background process_frame_and_tasks() is blocked
    pipeline.wait_blocked();

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_frames());

    TestHandler handler(pipeline);
    TestPipeline::Task task;

    // schedule() should see that pipeline is busy (locked by process_frame_and_tasks),
    // add the task to queue, and return
    pipeline.schedule(task, handler);

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // unblock background process_frame_and_tasks()
    pipeline.unblock_one_frame();

    // wait until process_frame_and_tasks() is finished
    // it should process the enqueued task
    fw.join();

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_frames());

    POINTERS_EQUAL(&task, handler.get_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());
}

TEST(task_pipeline, process_tasks_when_schedule_is_running) {
    TestPipeline pipeline(config);

    // next process_task_imp() call will block
    pipeline.block_tasks();

    TestHandler handler(pipeline);
    TestPipeline::Task task;

    // AsyncTaskScheduler will call schedule() from another thread
    // it will call process_task_imp() and block
    AsyncTaskScheduler ts(pipeline, task, &handler);
    ts.start();

    // wait until background schedule() is blocked
    pipeline.wait_blocked();

    POINTERS_EQUAL(NULL, handler.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // process_tasks() should see that pipeline is locked
    // (by background schedule()) and exit
    pipeline.process_tasks();

    // unblock blocked process_task_imp()
    pipeline.unblock_one_task();

    // wait until background schedule() finishes
    ts.join();

    POINTERS_EQUAL(&task, handler.get_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());
}

TEST(task_pipeline, process_tasks_when_another_process_tasks_is_running) {
    TestPipeline pipeline(config);

    pipeline.set_time(StartTime);

    // next process_task_imp() call will block
    pipeline.block_tasks();

    TestHandler handler1(pipeline);
    TestPipeline::Task task1;

    // AsyncTaskScheduler will call schedule() from another thread
    // it will call process_task_imp() and block
    AsyncTaskScheduler ts(pipeline, task1, &handler1);
    ts.start();

    // wait until background schedule() is blocked
    pipeline.wait_blocked();

    POINTERS_EQUAL(NULL, handler1.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    TestHandler handler2(pipeline);
    TestPipeline::Task task2;

    // this schedule() should see that the pipeline is busy (because it's
    // locked by process_task_imp()), add task to queue, and return
    pipeline.schedule(task2, handler2);

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // this deadline will be passed to schedule_task_processing()
    // in this case it means "process tasks immediately"
    pipeline.expect_sched_deadline(StartTime);

    // unblock blocked process_task_imp()
    pipeline.unblock_one_task();

    // wait until background schedule() finishes
    // it should process the first task, see that a new task was added,
    // call schedule_processing_tasks(), and return
    ts.join();

    POINTERS_EQUAL(&task1, handler1.wait_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // next process_task_imp() call will block (again)
    pipeline.block_tasks();

    // AsyncTaskProcessor will call process_tasks() from another thread
    // it will call process_task_imp() and block
    AsyncTaskProcessor tp(pipeline);
    tp.start();

    // wait until background process_tasks() is blocked
    pipeline.wait_blocked();

    // this process_tasks() should see that the pipeline is busy (because it's
    // locked by process_task_imp()) and return
    pipeline.process_tasks();

    POINTERS_EQUAL(NULL, handler2.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // unblock blocked process_task_imp()
    pipeline.unblock_one_task();

    // wait until background process_tasks() finishes
    // it should process task
    tp.join();

    POINTERS_EQUAL(&task2, handler2.get_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());
}

TEST(task_pipeline, process_tasks_when_processing_frame) {
    TestPipeline pipeline(config);

    audio::Frame frame(samples, FrameSize);
    fill_frame(frame, 0.1f, 0, FrameSize);
    pipeline.expect_frame(0.1f, FrameSize);

    // next process_frame_imp() call will block
    pipeline.block_frames();

    // AsyncFrameWriter will call process_frame_and_tasks() from background thread
    AsyncFrameWriter fw(pipeline, frame);
    fw.start();

    // wait until background process_frame_and_tasks() is blocked
    pipeline.wait_blocked();

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_frames());

    TestHandler handler(pipeline);
    TestPipeline::Task task;

    // schedule() should see that pipeline is busy (locked by process_frame_and_tasks),
    // add the task to queue, and return
    pipeline.schedule(task, handler);

    // this process_tasks() should see that pipeline is busy and just return
    pipeline.process_tasks();

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // unblock background process_frame_and_tasks()
    pipeline.unblock_one_frame();

    // wait until process_frame_and_tasks() is finished
    // it should process the enqueued task
    fw.join();

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_frames());

    POINTERS_EQUAL(&task, handler.get_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());
}

TEST(task_pipeline, process_tasks_interframe_deadline) {
    TestPipeline pipeline(config);

    pipeline.set_time(StartTime);

    audio::Frame frame(samples, FrameSize);
    fill_frame(frame, 0.1f, 0, FrameSize);
    pipeline.expect_frame(0.1f, FrameSize);

    // process frame and set inter-frame task processing deadline
    CHECK(pipeline.process_frame_and_tasks(frame));

    // next process_task_imp() call will block
    pipeline.block_tasks();

    TestHandler handler1(pipeline);
    TestPipeline::Task task1;

    // AsyncTaskScheduler will call schedule() from another thread
    // it will call process_task_imp() and block
    AsyncTaskScheduler ts(pipeline, task1, &handler1);
    ts.start();

    // wait until background schedule() is blocked
    pipeline.wait_blocked();

    POINTERS_EQUAL(NULL, handler1.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    TestHandler handler2a(pipeline);
    TestPipeline::Task task2a;
    TestHandler handler2b(pipeline);
    TestPipeline::Task task2b;

    TestHandler handler3(pipeline);
    TestPipeline::Task task3;

    // add tasks to the queue
    pipeline.schedule(task2a, handler2a);
    pipeline.schedule(task2b, handler2b);
    pipeline.schedule(task3, handler3);

    UNSIGNED_LONGS_EQUAL(4, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // this deadline will be passed to schedule_task_processing()
    // in this case it means "process tasks immediately"
    pipeline.expect_sched_deadline(StartTime);

    // unblock blocked process_task_imp()
    pipeline.unblock_one_task();

    // wait until background schedule() finishes
    // it should process the first task, see that a new task was added,
    // call schedule_processing_tasks(), and return
    ts.join();

    POINTERS_EQUAL(&task1, handler1.wait_task());

    UNSIGNED_LONGS_EQUAL(3, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // AsyncTaskProcessor will call process_tasks() from another thread
    // it will call process_task_imp() and block
    AsyncTaskProcessor tp(pipeline);
    tp.start();

    // wait until background process_tasks() is blocked
    pipeline.wait_blocked();

    // inter-frame task deadline not expired
    pipeline.set_time(StartTime + FrameSize * core::Microsecond - NoTaskProcessingGap / 2
                      - 1);

    // process task2a
    pipeline.unblock_one_task();

    // wait blocked on task2b
    pipeline.wait_blocked();

    // inter-frame task deadline expired
    pipeline.set_time(StartTime + FrameSize * core::Microsecond
                      - NoTaskProcessingGap / 2);

    // this deadline will be passed to schedule_task_processing()
    // if this deadline expires, it means that process_frame_and_tasks() was not
    // called for some reason and didn't process our tasks, so we should call
    // process_tasks()
    pipeline.expect_sched_deadline(StartTime + FrameSize * core::Microsecond
                                   + NoTaskProcessingGap / 2);

    // process task2b
    pipeline.unblock_one_task();

    // process_tasks() should see that deadline expired, exit and call
    // schedule_task_processing(), leaving task3 unprocessed
    tp.join();

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(3, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // new deadline not expired
    pipeline.set_time(StartTime + FrameSize * core::Microsecond);

    // will not process any tasks
    // will call schedule_task_processing() again
    pipeline.process_tasks();

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(3, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(3, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // new deadline expired
    pipeline.set_time(StartTime + FrameSize * core::Microsecond
                      + NoTaskProcessingGap / 2);

    // for task3
    pipeline.unblock_one_task();

    // will process task3
    pipeline.process_tasks();

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(4, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(3, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(3, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());

    POINTERS_EQUAL(&task2a, handler2a.get_task());
    POINTERS_EQUAL(&task2b, handler2b.get_task());

    POINTERS_EQUAL(&task3, handler3.get_task());
}

TEST(task_pipeline, process_frame_when_schedule_is_running) {
    TestPipeline pipeline(config);
    TestHandler handler(pipeline);

    // next process_task_imp() call will block
    pipeline.block_tasks();

    TestPipeline::Task task1;

    // AsyncTaskScheduler will call schedule() from another thread
    // it will call process_task_imp() and block
    AsyncTaskScheduler ts(pipeline, task1, &handler);
    ts.start();

    // wait until background schedule() is blocked
    pipeline.wait_blocked();

    POINTERS_EQUAL(NULL, handler.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    TestPipeline::Task task2;

    // this schedule() should see that the pipeline is busy (because it's
    // locked by another schedule), add task to queue, and return
    pipeline.schedule(task2, handler);

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());

    audio::Frame frame(samples, FrameSize);
    fill_frame(frame, 0.1f, 0, FrameSize);
    pipeline.expect_frame(0.1f, FrameSize);

    // next process_frame_imp() call will block
    pipeline.block_frames();

    // AsyncFrameWriter will call process_frame_and_tasks() from background thread
    // it will be blocked until process_task_imp() and schedule() return
    AsyncFrameWriter fw(pipeline, frame);
    fw.start();

    // wait until background process_frame_and_tasks() marks that a frame is pending
    while (pipeline.num_pending_frames() == 0) {
        core::sleep_for(core::Microsecond * 10);
    }

    // unblock blocked process_task_imp()
    pipeline.unblock_all_tasks();

    // wait until background schedule() finishes
    // it should process the first task, see that a new task was added, then see that
    // there is pending process_frame_and_tasks() call and thus don't call
    // schedule_processing_tasks() and just return
    ts.join();

    POINTERS_EQUAL(&task1, handler.wait_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_preemptions());

    // wait until background process_frame_and_tasks() calls process_frame_imp()
    // and blocks
    pipeline.wait_blocked();

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    // wake up process_frame_imp()
    pipeline.unblock_one_frame();

    // wait until background process_frame_and_tasks() finished
    // it should process the second task
    fw.join();

    POINTERS_EQUAL(&task2, handler.get_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_preemptions());
}

TEST(task_pipeline, process_frame_when_process_tasks_is_running) {
    TestPipeline pipeline(config);

    pipeline.set_time(StartTime);

    // next process_task_imp() call will block
    pipeline.block_tasks();

    TestHandler handler1(pipeline);
    TestPipeline::Task task1;

    // AsyncTaskScheduler will call schedule() from another thread
    // it will call process_task_imp() and block
    AsyncTaskScheduler ts(pipeline, task1, &handler1);
    ts.start();

    // wait until background schedule() is blocked
    pipeline.wait_blocked();

    POINTERS_EQUAL(NULL, handler1.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    TestHandler handler2(pipeline);
    TestPipeline::Task task2;

    // this schedule() should see that the pipeline is busy (because it's
    // locked by process_task_imp()), add task to queue, and return
    pipeline.schedule(task2, handler2);

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // this deadline will be passed to schedule_task_processing()
    // in this case it means "process tasks immediately"
    pipeline.expect_sched_deadline(StartTime);

    // unblock blocked process_task_imp()
    pipeline.unblock_one_task();

    // wait until background schedule() finishes
    // it should process the first task, see that a new task was added,
    // call schedule_processing_tasks(), and return
    ts.join();

    POINTERS_EQUAL(&task1, handler1.wait_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // next process_task_imp() call will block (again)
    pipeline.block_tasks();

    // AsyncTaskProcessor will call process_tasks() from another thread
    // it will call process_task_imp() and block
    AsyncTaskProcessor tp(pipeline);
    tp.start();

    // wait until background process_tasks() is blocked
    pipeline.wait_blocked();

    TestHandler handler3(pipeline);
    TestPipeline::Task task3;

    // this schedule() should see that the pipeline is busy (because it's
    // locked by process_task_imp()), add task to queue, and return
    pipeline.schedule(task3, handler3);

    POINTERS_EQUAL(NULL, handler2.get_task());
    POINTERS_EQUAL(NULL, handler3.get_task());

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // next process_frame_imp() call will block
    pipeline.block_frames();

    audio::Frame frame(samples, FrameSize);
    fill_frame(frame, 0.1f, 0, FrameSize);
    pipeline.expect_frame(0.1f, FrameSize);

    // AsyncFrameWriter will call process_frame_and_tasks() from background thread
    AsyncFrameWriter fw(pipeline, frame);
    fw.start();

    // wait until background process_frame_and_tasks() marks that a frame is pending
    while (pipeline.num_pending_frames() == 0) {
        core::sleep_for(core::Microsecond * 10);
    }

    // unblock blocked process_task_imp()
    pipeline.unblock_all_tasks();

    // wait until background process_tasks() finishes
    // it should process the second task, then see that a frame is pending and
    // exit without processing the third task
    tp.join();

    // wait until process_frame_and_tasks() calls process_frame_imp() and blocks
    pipeline.wait_blocked();

    POINTERS_EQUAL(&task2, handler2.get_task());
    POINTERS_EQUAL(NULL, handler3.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_preemptions());

    // unblock blocked process_frame_imp()
    pipeline.unblock_one_frame();

    // wait until background process_frame_and_tasks() finished
    // it should process the third task
    fw.join();

    POINTERS_EQUAL(&task3, handler3.get_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(3, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_preemptions());
}

TEST(task_pipeline, process_frame_max_samples_between_frames) {
    TestPipeline pipeline(config);

    pipeline.set_time(StartTime);

    audio::Frame frame(samples, MaxFrameSize * 2);
    fill_frame(frame, 0.1f, 0, MaxFrameSize);
    fill_frame(frame, 0.2f, MaxFrameSize, MaxFrameSize * 2);

    // first sub-frame
    pipeline.expect_frame(0.1f, MaxFrameSize);

    // next process_frame_imp() call will block
    pipeline.block_frames();

    // AsyncFrameWriter will call process_frame_and_tasks() from background thread
    AsyncFrameWriter fw(pipeline, frame);
    fw.start();

    // wait until background process_frame_and_tasks() is blocked
    pipeline.wait_blocked();

    TestHandler handler1a(pipeline);
    TestPipeline::Task task1a;
    TestHandler handler1b(pipeline);
    TestPipeline::Task task1b;

    TestHandler handler2a(pipeline);
    TestPipeline::Task task2a;
    TestHandler handler2b(pipeline);
    TestPipeline::Task task2b;

    TestHandler handler3(pipeline);
    TestPipeline::Task task3;

    // schedule() should add task to the queue and exit
    pipeline.schedule(task1a, handler1a);
    pipeline.schedule(task1b, handler1b);
    pipeline.schedule(task2a, handler2a);
    pipeline.schedule(task2b, handler2b);
    pipeline.schedule(task3, handler3);

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_frames());

    UNSIGNED_LONGS_EQUAL(5, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // next process_task_imp() will be blocked
    pipeline.block_tasks();

    // second sub-frame
    pipeline.expect_frame(0.2f, MaxFrameSize);

    // unblock one process_frame_imp()
    pipeline.unblock_one_frame();

    // wait we're blocked while processing task1a
    pipeline.wait_blocked();

    // sub-frame task deadline not expired
    pipeline.set_time(StartTime + MaxInframeProcessing / 2);

    // wait we're blocked while processing task1b
    pipeline.unblock_one_task();
    pipeline.wait_blocked();

    // sub-frame task deadline expired, should go to next sub-frame
    pipeline.set_time(StartTime + MaxInframeProcessing);

    // wait until blocked on second sub-frame
    pipeline.unblock_one_task();
    pipeline.wait_blocked();

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_frames());

    UNSIGNED_LONGS_EQUAL(3, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // emulate frame processing
    pipeline.set_time(StartTime + FrameProcessingTime);

    // unblock one process_frame_imp()
    pipeline.unblock_one_frame();

    // wait we're blocked while processing task2a
    pipeline.wait_blocked();

    // sub-frame task deadline not expired
    pipeline.set_time(StartTime + FrameProcessingTime + MaxInframeProcessing / 2);

    // wait we're blocked while processing task2b
    pipeline.unblock_one_task();
    pipeline.wait_blocked();

    // sub-frame task deadline expired, should exit without processing 3rd task
    pipeline.set_time(StartTime + FrameProcessingTime + MaxInframeProcessing);

    // pipeline should schedule task processing to be called immediately
    pipeline.expect_sched_deadline(StartTime + FrameProcessingTime
                                   + MaxInframeProcessing);

    // unblock one process_frame_imp()
    pipeline.unblock_one_task();

    // wait background process_frame_and_tasks() finises
    fw.join();

    // two sub-frames
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_processed_frames());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(4, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(4, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // unblock one process_frame_imp()
    pipeline.unblock_one_task();

    // this should process the last task
    pipeline.process_tasks();

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_processed_frames());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(5, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(4, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());

    POINTERS_EQUAL(&task1a, handler1a.get_task());
    POINTERS_EQUAL(&task1b, handler1b.get_task());

    POINTERS_EQUAL(&task2a, handler2a.get_task());
    POINTERS_EQUAL(&task2b, handler2b.get_task());

    POINTERS_EQUAL(&task3, handler3.get_task());
}

TEST(task_pipeline, process_frame_min_samples_between_frames) {
    TestPipeline pipeline(config);

    pipeline.set_time(StartTime);

    // process_frame_imp() call will block
    pipeline.block_frames();

    // first frame
    audio::Frame frame1(samples, MinFrameSize / 2);
    fill_frame(frame1, 0.1f, 0, MinFrameSize / 2);
    pipeline.expect_frame(0.1f, MinFrameSize / 2);

    // call process_frame_and_tasks(frame1) from background thread
    AsyncFrameWriter fw(pipeline, frame1);
    fw.start();

    // wait until background process_frame_and_tasks() is blocked
    pipeline.wait_blocked();

    TestHandler handler1(pipeline);
    TestPipeline::Task task1;
    TestHandler handler2(pipeline);
    TestPipeline::Task task2;

    // schedule() should add task to the queue and exit
    pipeline.schedule(task1, handler1);
    pipeline.schedule(task2, handler2);

    // unblock process_frame_and_tasks() and wait it finishes
    // it should not process any tasks because the frame is too small and tasks
    // should not be processed in-frame until at least MinFrameSize samples
    // is processed
    // it should call schedule_task_processing()
    pipeline.expect_sched_deadline(StartTime);
    pipeline.unblock_all_frames();
    fw.join();

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_frames());

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // second frame
    audio::Frame frame2(samples, MinFrameSize / 2);
    fill_frame(frame2, 0.2f, 0, MinFrameSize / 2);
    pipeline.expect_frame(0.2f, MinFrameSize / 2);

    // now we have processed MinFrameSize samples, pipeline should call
    // cancel_task_processing() and process pending task1 and task2
    pipeline.process_frame_and_tasks(frame2);

    CHECK(task1.success());
    CHECK(task2.success());

    POINTERS_EQUAL(&task1, handler1.get_task());
    POINTERS_EQUAL(&task2, handler2.get_task());

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_processed_frames());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_cancellations());
}

TEST(task_pipeline, schedule_from_completion_handler_called_in_place) {
    TestPipeline pipeline(config);

    TestPipeline::Task task1;
    TestPipeline::Task task2;

    TestHandler handler(pipeline);

    // schedule_task_processing() should be called with zero delay, i.e.
    // "process tasks immediately"
    pipeline.set_time(StartTime);
    pipeline.expect_sched_deadline(StartTime);

    // completion handler will schedule() task2
    handler.set_next_task(task2);

    // schedule() should process task1 in-place and call completion handler
    // task2 shoyld be added to queue and not processed
    // schedule_task_processing() should be called to process task2 asynchronously
    pipeline.schedule(task1, handler);

    CHECK(handler.get_task() == &task1);

    CHECK(task1.success());
    CHECK(!task2.success());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());

    // should process task2
    pipeline.process_tasks();

    CHECK(handler.get_task() == &task2);

    CHECK(task1.success());
    CHECK(task2.success());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_preemptions());
}

TEST(task_pipeline, schedule_from_completion_handler_called_from_process_tasks) {
    TestPipeline pipeline(config);

    pipeline.set_time(StartTime);

    // next process_task_imp() call will block
    pipeline.block_tasks();

    TestHandler handler1(pipeline);
    TestPipeline::Task task1;

    // AsyncTaskScheduler will call schedule() from another thread
    // it will call process_task_imp() and block
    AsyncTaskScheduler ts(pipeline, task1, &handler1);
    ts.start();

    // wait until background schedule() is blocked
    pipeline.wait_blocked();

    POINTERS_EQUAL(NULL, handler1.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    TestHandler handler2(pipeline);
    TestPipeline::Task task2;

    // this schedule() should see that the pipeline is busy (because it's
    // locked by process_task_imp()), add task to queue, and return
    pipeline.schedule(task2, handler2);

    // unblock blocked schedule() and wait it finishes
    // it should call schedule_task_processing()
    pipeline.expect_sched_deadline(StartTime);
    pipeline.unblock_all_tasks();
    ts.join();

    POINTERS_EQUAL(&task1, handler1.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // task2 completion handler will schedule task3
    TestPipeline::Task task3;
    handler2.set_next_task(task3);

    // this should execute task2 and its completion handler
    // task3 should be added to the queue and then immediately processed
    pipeline.process_tasks();

    CHECK(task2.success());
    CHECK(task3.success());

    POINTERS_EQUAL(&task3, handler2.get_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(3, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());
}

TEST(task_pipeline, schedule_from_completion_handler_called_from_process_frame) {
    TestPipeline pipeline(config);

    pipeline.set_time(StartTime);

    // next process_task_imp() call will block
    pipeline.block_tasks();

    TestHandler handler1(pipeline);
    TestPipeline::Task task1;

    // AsyncTaskScheduler will call schedule() from another thread
    // it will call process_task_imp() and block
    AsyncTaskScheduler ts(pipeline, task1, &handler1);
    ts.start();

    // wait until background schedule() is blocked
    pipeline.wait_blocked();

    POINTERS_EQUAL(NULL, handler1.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    TestHandler handler2(pipeline);
    TestPipeline::Task task2;

    // this schedule() should see that the pipeline is busy (because it's
    // locked by process_task_imp()), add task to queue, and return
    pipeline.schedule(task2, handler2);

    // unblock blocked schedule() and wait it finishes
    // it should call schedule_task_processing()
    pipeline.expect_sched_deadline(StartTime);
    pipeline.unblock_all_tasks();
    ts.join();

    POINTERS_EQUAL(&task1, handler1.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // task2 completion handler will schedule task3
    TestPipeline::Task task3;
    handler2.set_next_task(task3);

    audio::Frame frame(samples, FrameSize);
    fill_frame(frame, 0.1f, 0, FrameSize);
    pipeline.expect_frame(0.1f, FrameSize);

    // this should call cancel_task_processing() and then execute task2 and
    // its completion handler
    // task3 should be added to the queue and then immediately processed
    CHECK(pipeline.process_frame_and_tasks(frame));

    CHECK(task2.success());
    CHECK(task3.success());

    POINTERS_EQUAL(&task3, handler2.get_task());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(3, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(2, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_cancellations());
}

TEST(task_pipeline, schedule_and_wait_until_process_tasks_called) {
    TestPipeline pipeline(config);
    TestHandler handler(pipeline);

    pipeline.set_time(StartTime);

    // next process_task_imp() call will block
    pipeline.block_tasks();

    TestPipeline::Task task1;

    // AsyncTaskScheduler will call schedule() from another thread
    // it will call process_task_imp() and block
    AsyncTaskScheduler ts1(pipeline, task1, &handler);
    ts1.start();

    // wait until background schedule() is blocked
    pipeline.wait_blocked();

    POINTERS_EQUAL(NULL, handler.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    TestPipeline::Task task2;

    // this schedule() should see that the pipeline is busy (because it's
    // locked by process_task_imp()), add task to queue, and return
    pipeline.schedule(task2, handler);

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // unblock blocked schedule() and wait it finishes
    // it should call schedule_task_processing()
    pipeline.expect_sched_deadline(StartTime);
    pipeline.unblock_all_tasks();
    ts1.join();

    POINTERS_EQUAL(&task1, handler.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // this will call schedule_and_wait() from another thread
    // it will add task to the queue and wait until we call process_tasks()
    // it shouldn't call schedule_task_processing() because it's already called
    TestPipeline::Task task3a;
    AsyncTaskScheduler ts3a(pipeline, task3a, NULL);
    ts3a.start();

    // another concurrent schedule_and_wait()
    TestPipeline::Task task3b;
    AsyncTaskScheduler ts3b(pipeline, task3b, NULL);
    ts3b.start();

    while (pipeline.num_pending_tasks() != 3) {
        core::sleep_for(core::Microsecond * 10);
    }

    UNSIGNED_LONGS_EQUAL(3, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // this should process task2, task3a, and task3b
    // both background schedule_and_wait() calls should finish
    pipeline.process_tasks();

    // wait schedule_and_wait() finished
    ts3a.join();
    ts3b.join();

    POINTERS_EQUAL(&task2, handler.get_task());

    CHECK(task3a.success());
    CHECK(task3b.success());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(4, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(3, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());
}

TEST(task_pipeline, schedule_and_wait_until_process_frame_called) {
    TestPipeline pipeline(config);
    TestHandler handler(pipeline);

    pipeline.set_time(StartTime);

    // next process_task_imp() call will block
    pipeline.block_tasks();

    TestPipeline::Task task1;

    // AsyncTaskScheduler will call schedule() from another thread
    // it will call process_task_imp() and block
    AsyncTaskScheduler ts1(pipeline, task1, &handler);
    ts1.start();

    // wait until background schedule() is blocked
    pipeline.wait_blocked();

    POINTERS_EQUAL(NULL, handler.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    TestPipeline::Task task2;

    // this schedule() should see that the pipeline is busy (because it's
    // locked by process_task_imp()), add task to queue, and return
    pipeline.schedule(task2, handler);

    UNSIGNED_LONGS_EQUAL(2, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // unblock blocked schedule() and wait it finishes
    // it should call schedule_task_processing()
    pipeline.expect_sched_deadline(StartTime);
    pipeline.unblock_all_tasks();
    ts1.join();

    POINTERS_EQUAL(&task1, handler.get_task());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    // this will call schedule_and_wait() from another thread
    // it will add task to the queue and wait until we call process_tasks()
    // it shouldn't call schedule_task_processing() because it's already called
    TestPipeline::Task task3a;
    AsyncTaskScheduler ts3a(pipeline, task3a, NULL);
    ts3a.start();

    // another concurrent schedule_and_wait()
    TestPipeline::Task task3b;
    AsyncTaskScheduler ts3b(pipeline, task3b, NULL);
    ts3b.start();

    while (pipeline.num_pending_tasks() != 3) {
        core::sleep_for(core::Microsecond * 10);
    }

    UNSIGNED_LONGS_EQUAL(3, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_sched_cancellations());

    audio::Frame frame(samples, FrameSize);
    fill_frame(frame, 0.1f, 0, FrameSize);
    pipeline.expect_frame(0.1f, FrameSize);

    // this should call cancel_task_scheduling() and process task2 and task3
    // both background schedule_and_wait() calls should finish
    CHECK(pipeline.process_frame_and_tasks(frame));

    // wait schedule_and_wait() finished
    ts3a.join();
    ts3b.join();

    POINTERS_EQUAL(&task2, handler.get_task());

    CHECK(task3a.success());
    CHECK(task3b.success());

    UNSIGNED_LONGS_EQUAL(0, pipeline.num_pending_tasks());
    UNSIGNED_LONGS_EQUAL(4, pipeline.num_processed_tasks());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_tasks_processed_in_sched());
    UNSIGNED_LONGS_EQUAL(3, pipeline.num_tasks_processed_in_frame());
    UNSIGNED_LONGS_EQUAL(0, pipeline.num_tasks_processed_in_proc());

    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_calls());
    UNSIGNED_LONGS_EQUAL(1, pipeline.num_sched_cancellations());
}

} // namespace pipeline
} // namespace roc
