/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/pipeline_loop.h
//! @brief Base class for pipelines.

#ifndef ROC_PIPELINE_PIPELINE_LOOP_H_
#define ROC_PIPELINE_PIPELINE_LOOP_H_

#include "roc_audio/frame.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/atomic.h"
#include "roc_core/mpsc_queue.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/seqlock.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"
#include "roc_pipeline/ipipeline_task_completer.h"
#include "roc_pipeline/ipipeline_task_scheduler.h"
#include "roc_pipeline/pipeline_task.h"
#include "roc_status/status_code.h"

namespace roc {
namespace pipeline {

//! Pipeline loop task processing parameters.
struct PipelineLoopConfig {
    //! Enable precise task scheduling mode (default).
    //! The other settings have effect only when this is set to true.
    //! When enabled, pipeline processes tasks in dedicated time intervals between
    //! sub-frame and between frames, trying to prevent time collisions between
    //! task and frame processing.
    bool enable_precise_task_scheduling;

    //! Minimum frame duration between processing tasks.
    //! In-frame task processing does not happen until at least given number
    //! of samples is processed.
    //! Set to zero to allow task processing between frames of any size.
    core::nanoseconds_t min_frame_length_between_tasks;

    //! Maximum frame duration between processing tasks.
    //! If the frame is larger than this size, it is split into multiple subframes
    //! to allow task processing between the sub-frames.
    //! Set to zero to disable frame splitting.
    core::nanoseconds_t max_frame_length_between_tasks;

    //! Maximum task processing duration happening immediately after processing a frame.
    //! If this period expires and there are still pending tasks, asynchronous
    //! task processing is scheduled.
    //! At least one task is always processed after each frame, even if this
    //! setting is too small.
    core::nanoseconds_t max_inframe_task_processing;

    //! Time interval during which no task processing is allowed.
    //! This setting is used to prohibit task processing during the time when
    //! next read() or write() call is expected.
    //! Since it can not be calculated absolutely precisely, and there is always
    //! thread switch overhead, scheduler jitter clock drift, we use a wide interval.
    core::nanoseconds_t task_processing_prohibited_interval;

    PipelineLoopConfig()
        : enable_precise_task_scheduling(true)
        , min_frame_length_between_tasks(200 * core::Microsecond)
        , max_frame_length_between_tasks(1 * core::Millisecond)
        , max_inframe_task_processing(20 * core::Microsecond)
        , task_processing_prohibited_interval(200 * core::Microsecond) {
    }
};

//! Base class for task-based pipelines.
//!
//! Frames, tasks, and threads
//! --------------------------
//!
//! The pipeline processes frames and tasks. This processing is serialized. At every
//! moment, the pipeline is either processing a frame, processing a task, or doing
//! nothing.
//!
//! The pipeline does not have its own thread. Both frame and task processing happens
//! when the user calls one of the pipeline methods, in the context of the caller thread.
//! Methods may be called from different threads, concurrently. This complicates the
//! implementation, but allows to have different thread layouts for different use cases.
//!
//! Precise task scheduling
//! -----------------------
//!
//! This class implements "precise task scheduling" feature, which tries to schedule task
//! processing intervals smartly, to prevent time collisions with frame processing and
//! keep frame processing timings unaffected.
//!
//! Precise task scheduling is enabled by default, but can be disabled via config. When
//! disabled, no special scheduling is performed and frame and task processing compete
//! each other for the exclusive access to the pipeline.
//!
//! The sections below describe various aspects of the implementation.
//!
//! Task processing time slices
//! ---------------------------
//!
//! Tasks are processed between frames on dedicated time slices, to ensure that the
//! task processing wont delay frame processing, which should be as close to real-time
//! as possible.
//!
//! If frame is too large, it's split into sub-frames, to allow task processing between
//! these sub-frames. This is needed to ensure that the task processing delay would not
//! be too large, at least while there are not too much tasks.
//!
//! If frames are too small, tasks are processing only after some of the frames instead
//! of after every frame. This is needed to reduce task processing overhead when using
//! tiny frames.
//!
//! There are two types of time slices dedicated for task processing:
//!  - in-frame task processing: short intervals between sub-frames
//!    (inside process_frame_and_tasks())
//!  - inter-frame longer intervals between frames
//!    (inside process_tasks())
//!
//! process_frame_and_tasks() calls are to be driven by the user-defined pipeline
//! clock. It should be called exactly when it's time to process more samples. Our
//! goal is to provide it exclusive access to the pipeline as fast as possible
//! immediately after it's called.
//!
//! process_tasks() should be called by user when there are pending tasks that should
//! be processed and when no concurrent process_frame_and_tasks() call is running.
//! Our goal is to notify the user if and when it should be called.
//!
//! Asynchronous task processing
//! ----------------------------
//!
//! Since pipeline does not have its own thread, it can't schedule process_tasks()
//! invocation by its own. Instead, it relies on the user-provided IPipelineTaskScheduler
//! object.
//!
//! When the pipeline wants to schedule asynchronous process_tasks() invocation, it
//! calls IPipelineTaskScheduler::schedule_task_processing(). It's up to the user when and
//! on which thread to invoke process_tasks(), but pipeline gives a hint with the ideal
//! invocation time.
//!
//! The pipeline may also cancel the scheduled task processing by invoking
//! IPipelineTaskScheduler::cancel_task_processing().
//!
//! In-place task processing
//! ------------------------
//!
//! If schedule() or schedule_and_wait() is called when the task queue is empty and the
//! current time point belongs to the task processing time slice, the new task is
//! processed in-place without waiting for the next process_frame_and_tasks() or
//! process_tasks() invocation. This allows to avoid extra delays and thread switches
//! when possible.
//!
//! Processing priority
//! -------------------
//!
//! When process_frame_and_tasks() is called, it increments pending_frame_ atomic
//! and blocks on pipeline_mutex_. The non-zero atomic indicates that a frame needs
//! to be processed as soon as possible and other methods should give it a way.
//!
//! When process_frame_and_tasks() is called, it also cancels any scheduled
//! asynchronous task processing before starting processing the frame and tasks.
//! Before exiting, process_frame_and_tasks() checks if there are still some pending
//! tasks and if necessary, schedules asynchronous execution again.
//!
//! When process_tasks() is processing asynchronous tasks, but detects that
//! process_frame_and_tasks() was invoked concurrently from another thread, it gives
//! it a way and exits. process_frame_and_tasks() will process the frame and some of
//! the remaining tasks, and if there are even more tasks remaining, it will invoke
//! schedule_task_processing() to allow process_tasks() to continue.
//!
//! When schedule() and process_tasks() want to invoke schedule_task_processing(), but
//! detect that process_frame_and_tasks() was invoked concurrently from another thread,
//! they give it a way and don't call schedule_task_processing(), assuming that
//! process_frame_and_tasks() will either process all tasks or call
//! schedule_task_processing() by itself.
//!
//! Locking rules
//! -------------
//!
//! pipeline_mutex_ protects the internal pipeline state. It should be acquired to
//! process a frame or a task.
//!
//! scheduler_mutex_ protects IPipelineTaskScheduler invocations. It should be acquired to
//! schedule or cancel asynchronous task processing.
//!
//! If pipeline_mutex_ is locked, it's guaranteed that the thread locking it will
//! check pending tasks after unlocking the mutex and will either process them or
//! scheduler asynchronous processing.
//!
//! If scheduler_mutex_ is locked, it's guaranteed that the thread locking it will
//! either schedule or cancel asynchronous task processing, depending on whether
//! there are pending tasks and frames.
//!
//! Lock-free operations
//! --------------------
//!
//! schedule() and process_tasks() methods are lock-free. Also, they're either completely
//! wait-free or "mostly" wait-free (i.e. on the fast path), depending on the hardware
//! architecture (see comments for core::MpscQueue).
//!
//! In practice it means that when running concurrently with other PipelineLoop method
//! invocations, they never block waiting for other threads, and usually even don't spin.
//!
//! This is archived by using a lock-free queue for tasks, atomics for 32-bit counters,
//! seqlocks for 64-bit counters (which are reduced to atomics on 64-bit CPUs), always
//! using try_lock() for mutexes and delaying the work if the mutex can't be acquired,
//! and using semaphores instead of condition variables for signaling (which don't
//! require blocking on mutex, at least on modern platforms; e.g. on glibc they're
//! implemented using an atomic and a futex).
//!
//! process_frame_and_tasks() is not lock-free because it has to acquire the pipeline
//! mutex and can't delay its work. However, the precise task scheduling feature does it
//! best to ensure that the pipeline mutex will be unlocked when process_frame_and_tasks()
//! is invoked, thus in most cases it wont block or wait too.
//!
//! This approach helps us with our global goal of making all inter-thread interactions
//! mostly wait-free, so that one thread is never or almost never blocked when another
//! thread is blocked, preempted, or busy.
//!
//! Benchmarks
//! ----------
//!
//! PipelineLoop is covered with two groups of benchmarks:
//!  - bench_pipeline_loop_peak_load.cpp measures frame and task processing delays with
//!    or without task load and with or without precise task scheduling feature;
//!  - bench_pipeline_loop_contention.cpp measures scheduling times under different
//!    contention levels.
//!
//! You can run them using "roc-bench-pipeline" command. For further details, see
//! comments in the source code of the benchmarks.
class PipelineLoop : public core::NonCopyable<> {
public:
    //! Enqueue a task for asynchronous execution.
    void schedule(PipelineTask& task, IPipelineTaskCompleter& completer);

    //! Enqueue a task for asynchronous execution and wait until it finishes.
    //! @returns false if the task fails.
    bool schedule_and_wait(PipelineTask& task);

    //! Process some of the enqueued tasks, if any.
    void process_tasks();

protected:
    //! Pipeline direction.
    enum Direction {
        Dir_ReadFrames,  //!< Reading frames from pipeline.
        Dir_WriteFrames, //!< Writing frames to pipeline.
    };

    //! Task processing statistics.
    struct Stats {
        //! Total number of tasks processed.
        uint64_t task_processed_total;

        //! Number of tasks processed directly in schedule() or schedule_and_wait().
        uint64_t task_processed_in_place;

        //! Number of tasks processed in process_frame_and_tasks().
        uint64_t task_processed_in_frame;

        //! Number of times when other method was preempted by process_frame_and_tasks().
        uint64_t preemptions;

        //! Number of time when schedule_task_processing() was called.
        uint64_t scheduler_calls;

        //! Number of time when cancel_task_processing() was called.
        uint64_t scheduler_cancellations;

        Stats()
            : task_processed_total(0)
            , task_processed_in_place(0)
            , task_processed_in_frame(0)
            , preemptions(0)
            , scheduler_calls(0)
            , scheduler_cancellations(0) {
        }
    };

    //! Initialization.
    PipelineLoop(IPipelineTaskScheduler& scheduler,
                 const PipelineLoopConfig& config,
                 const audio::SampleSpec& sample_spec,
                 core::IPool& frame_pool,
                 core::IPool& frame_buffer_pool,
                 Direction direction);

    virtual ~PipelineLoop();

    //! How much pending tasks are there.
    size_t num_pending_tasks() const;

    //! How much pending frames are there.
    size_t num_pending_frames() const;

    //! Get task processing statistics.
    //! Returned object can't be accessed concurrently with other methods.
    const Stats& stats_ref() const;

    //! Split frame and process subframes and some of the enqueued tasks.
    ROC_ATTR_NODISCARD status::StatusCode
    process_subframes_and_tasks(audio::Frame& frame,
                                packet::stream_timestamp_t frame_duration,
                                audio::FrameReadMode mode);

    //! Get current time.
    virtual core::nanoseconds_t timestamp_imp() const = 0;

    //! Get current thread id.
    virtual uint64_t tid_imp() const = 0;

    //! Read or write subframe.
    virtual status::StatusCode
    process_subframe_imp(audio::Frame& frame,
                         packet::stream_timestamp_t frame_duration,
                         audio::FrameReadMode frame_mode) = 0;

    //! Process task.
    virtual bool process_task_imp(PipelineTask& task) = 0;

private:
    enum ProcState { ProcNotScheduled, ProcScheduled, ProcRunning };

    status::StatusCode
    process_subframes_and_tasks_simple_(audio::Frame& frame,
                                        packet::stream_timestamp_t frame_duration,
                                        audio::FrameReadMode frame_mode);
    status::StatusCode
    process_subframes_and_tasks_precise_(audio::Frame& frame,
                                         packet::stream_timestamp_t frame_duration,
                                         audio::FrameReadMode frame_mode);

    bool schedule_and_maybe_process_task_(PipelineTask& task);
    bool maybe_process_tasks_();

    void schedule_async_task_processing_();
    void cancel_async_task_processing_();

    void process_task_(PipelineTask& task, bool notify);
    status::StatusCode process_next_subframe_(audio::Frame& frame,
                                              packet::stream_timestamp_t* frame_pos,
                                              packet::stream_timestamp_t frame_duration,
                                              audio::FrameReadMode frame_mode);
    status::StatusCode
    make_and_process_subframe_(audio::Frame& frame,
                               packet::stream_timestamp_t frame_duration,
                               packet::stream_timestamp_t subframe_pos,
                               packet::stream_timestamp_t subframe_duration,
                               audio::FrameReadMode subframe_mode);

    bool start_subframe_task_processing_();
    bool subframe_task_processing_allowed_(core::nanoseconds_t next_frame_deadline) const;

    core::nanoseconds_t
    update_next_frame_deadline_(core::nanoseconds_t frame_start_time,
                                packet::stream_timestamp_t frame_duration);
    bool
    interframe_task_processing_allowed_(core::nanoseconds_t next_frame_deadline) const;

    void report_stats_();

    // configuration
    const PipelineLoopConfig config_;
    const Direction direction_;

    const audio::SampleSpec sample_spec_;

    const packet::stream_timestamp_t min_samples_between_tasks_;
    const packet::stream_timestamp_t max_samples_between_tasks_;

    const core::nanoseconds_t no_task_proc_half_interval_;

    // sub-frame allocation
    audio::FrameFactory frame_factory_;
    audio::FramePtr subframe_;

    // used to schedule asynchronous work
    IPipelineTaskScheduler& scheduler_;

    // protects pipeline state
    core::Mutex pipeline_mutex_;

    // protects IPipelineTaskScheduler
    core::Mutex scheduler_mutex_;

    // lock-free queue of pending tasks
    core::MpscQueue<PipelineTask, core::NoOwnership> task_queue_;

    // counter of pending tasks
    core::Atomic<int> pending_tasks_;

    // counter of pending process_frame_and_tasks() calls blocked on pipeline_mutex_
    core::Atomic<int> pending_frames_;

    // asynchronous processing state
    core::Atomic<int> processing_state_;

    // tid of last thread that performed frame processing
    core::Seqlock<uint64_t> frame_processing_tid_;

    // when next frame is expected to be started
    core::Seqlock<core::nanoseconds_t> next_frame_deadline_;

    // when task processing before next sub-frame ends
    core::nanoseconds_t subframe_tasks_deadline_;

    // number of samples processed since last in-frame task processing
    packet::stream_timestamp_t samples_processed_;

    // did we accumulate enough samples in samples_processed_
    bool enough_samples_to_process_tasks_;

    // task processing statistics
    core::RateLimiter rate_limiter_;
    Stats stats_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_PIPELINE_LOOP_H_
