/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/pipeline_loop.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace pipeline {

namespace {

const core::nanoseconds_t StatsReportInterval = core::Minute;

} // namespace

PipelineLoop::PipelineLoop(IPipelineTaskScheduler& scheduler,
                           const PipelineLoopConfig& config,
                           const audio::SampleSpec& sample_spec,
                           core::IPool& frame_pool,
                           core::IPool& frame_buffer_pool,
                           Direction direction)
    : config_(config)
    , direction_(direction)
    , sample_spec_(sample_spec)
    , min_samples_between_tasks_(
          sample_spec.ns_2_stream_timestamp(config.min_frame_length_between_tasks))
    , max_samples_between_tasks_(
          sample_spec.ns_2_stream_timestamp(config.max_frame_length_between_tasks))
    , no_task_proc_half_interval_(config.task_processing_prohibited_interval / 2)
    , frame_factory_(frame_pool, frame_buffer_pool)
    , scheduler_(scheduler)
    , pending_tasks_(0)
    , pending_frames_(0)
    , processing_state_(ProcNotScheduled)
    , frame_processing_tid_(0)
    , next_frame_deadline_(0)
    , subframe_tasks_deadline_(0)
    , samples_processed_(0)
    , enough_samples_to_process_tasks_(false)
    , rate_limiter_(StatsReportInterval) {
}

PipelineLoop::~PipelineLoop() {
    if (pending_tasks_ != 0) {
        roc_panic(
            "pipeline loop: attempt to destroy pipeline before finishing all tasks");
    }
}

const PipelineLoop::Stats& PipelineLoop::stats_ref() const {
    return stats_;
}

size_t PipelineLoop::num_pending_tasks() const {
    return (size_t)pending_tasks_;
}

size_t PipelineLoop::num_pending_frames() const {
    return (size_t)pending_frames_;
}

void PipelineLoop::schedule(PipelineTask& task, IPipelineTaskCompleter& completer) {
    if (task.state_ != PipelineTask::StateNew) {
        roc_panic("pipeline loop: attempt to schedule task more than once");
    }

    task.completer_ = &completer;

    schedule_and_maybe_process_task_(task);
}

bool PipelineLoop::schedule_and_wait(PipelineTask& task) {
    if (task.state_ != PipelineTask::StateNew) {
        roc_panic("pipeline loop: attempt to schedule task more than once");
    }

    task.completer_ = NULL;

    if (!task.sem_) {
        task.sem_.reset(new (task.sem_) core::Semaphore);
    }

    const bool processed = schedule_and_maybe_process_task_(task);

    if (!processed) {
        task.sem_->wait();
    }

    return task.success_;
}

bool PipelineLoop::schedule_and_maybe_process_task_(PipelineTask& task) {
    task.state_ = PipelineTask::StateScheduled;

    if (++pending_tasks_ != 1) {
        task_queue_.push_back(task);
        return false;
    }

    core::nanoseconds_t next_frame_deadline;
    if (!next_frame_deadline_.try_load(next_frame_deadline)) {
        task_queue_.push_back(task);
        return false;
    }

    if (!interframe_task_processing_allowed_(next_frame_deadline)) {
        task_queue_.push_back(task);

        if (pending_frames_ == 0) {
            schedule_async_task_processing_();
        }

        return false;
    }

    if (!pipeline_mutex_.try_lock()) {
        task_queue_.push_back(task);
        return false;
    }

    process_task_(task, false);
    --pending_tasks_;

    stats_.task_processed_total++;
    stats_.task_processed_in_place++;

    const int n_pending_frames = pending_frames_;
    if (n_pending_frames != 0) {
        stats_.preemptions++;
    }

    pipeline_mutex_.unlock();

    if (n_pending_frames == 0 && pending_tasks_ != 0) {
        schedule_async_task_processing_();
    }

    return true;
}

void PipelineLoop::process_tasks() {
    const bool need_reschedule = maybe_process_tasks_();

    processing_state_ = ProcNotScheduled;

    if (need_reschedule) {
        schedule_async_task_processing_();
    }
}

bool PipelineLoop::maybe_process_tasks_() {
    core::nanoseconds_t next_frame_deadline;
    if (!next_frame_deadline_.try_load(next_frame_deadline)) {
        return false;
    }

    if (!pipeline_mutex_.try_lock()) {
        return false;
    }

    processing_state_ = ProcRunning;

    int n_pending_frames = 0;

    for (;;) {
        if (!interframe_task_processing_allowed_(next_frame_deadline)) {
            break;
        }

        if ((n_pending_frames = pending_frames_) != 0) {
            break;
        }

        PipelineTask* task = task_queue_.try_pop_front_exclusive();
        if (!task) {
            break;
        }

        process_task_(*task, true);
        --pending_tasks_;

        stats_.task_processed_total++;
    }

    if (n_pending_frames != 0) {
        stats_.preemptions++;
    }

    pipeline_mutex_.unlock();

    return (n_pending_frames == 0 && pending_tasks_ != 0);
}

status::StatusCode
PipelineLoop::process_subframes_and_tasks(audio::Frame& frame,
                                          packet::stream_timestamp_t frame_duration,
                                          audio::FrameReadMode frame_mode) {
    if (config_.enable_precise_task_scheduling) {
        return process_subframes_and_tasks_precise_(frame, frame_duration, frame_mode);
    }
    return process_subframes_and_tasks_simple_(frame, frame_duration, frame_mode);
}

status::StatusCode PipelineLoop::process_subframes_and_tasks_simple_(
    audio::Frame& frame,
    packet::stream_timestamp_t frame_duration,
    audio::FrameReadMode frame_mode) {
    ++pending_frames_;

    cancel_async_task_processing_();

    pipeline_mutex_.lock();

    const status::StatusCode frame_status =
        process_subframe_imp(frame, frame_duration, frame_mode);

    pipeline_mutex_.unlock();

    if (--pending_frames_ == 0 && pending_tasks_ != 0) {
        schedule_async_task_processing_();
    }

    return frame_status;
}

status::StatusCode PipelineLoop::process_subframes_and_tasks_precise_(
    audio::Frame& frame,
    packet::stream_timestamp_t frame_duration,
    audio::FrameReadMode frame_mode) {
    ++pending_frames_;

    const core::nanoseconds_t frame_start_time = timestamp_imp();

    cancel_async_task_processing_();

    pipeline_mutex_.lock();

    core::nanoseconds_t next_frame_deadline = 0;

    packet::stream_timestamp_t frame_pos = 0;
    status::StatusCode frame_status = status::NoStatus;

    for (;;) {
        const bool first_iteration = (frame_pos == 0);

        frame_status =
            process_next_subframe_(frame, &frame_pos, frame_duration, frame_mode);

        if (first_iteration) {
            next_frame_deadline =
                update_next_frame_deadline_(frame_start_time, frame_duration);
        }

        if (start_subframe_task_processing_()) {
            while (PipelineTask* task = task_queue_.try_pop_front_exclusive()) {
                process_task_(*task, true);
                --pending_tasks_;

                stats_.task_processed_total++;
                stats_.task_processed_in_frame++;

                if (!subframe_task_processing_allowed_(next_frame_deadline)) {
                    break;
                }
            }
        }

        if (frame_status != status::StatusOK || frame_pos == frame_duration) {
            break;
        }
    }

    report_stats_();

    frame_processing_tid_.exclusive_store(tid_imp());

    pipeline_mutex_.unlock();

    if (--pending_frames_ == 0 && pending_tasks_ != 0) {
        schedule_async_task_processing_();
    }

    return frame_status;
}

void PipelineLoop::schedule_async_task_processing_() {
    core::nanoseconds_t next_frame_deadline;
    if (!next_frame_deadline_.try_load(next_frame_deadline)) {
        return;
    }

    if (!scheduler_mutex_.try_lock()) {
        return;
    }

    if (processing_state_ == ProcNotScheduled) {
        core::nanoseconds_t deadline = 0;

        if (config_.enable_precise_task_scheduling) {
            const core::nanoseconds_t now = timestamp_imp();

            if (now < (next_frame_deadline - no_task_proc_half_interval_)) {
                deadline = 0;
            } else if (now < (next_frame_deadline + no_task_proc_half_interval_)) {
                deadline = (next_frame_deadline + no_task_proc_half_interval_);
            } else {
                deadline = 0;
            }
        }

        scheduler_.schedule_task_processing(*this, deadline);
        stats_.scheduler_calls++;

        processing_state_ = ProcScheduled;
    }

    scheduler_mutex_.unlock();

    if (pending_frames_ != 0) {
        cancel_async_task_processing_();
    }
}

void PipelineLoop::cancel_async_task_processing_() {
    if (!scheduler_mutex_.try_lock()) {
        return;
    }

    if (processing_state_ == ProcScheduled) {
        scheduler_.cancel_task_processing(*this);
        stats_.scheduler_cancellations++;

        processing_state_ = ProcNotScheduled;
    }

    scheduler_mutex_.unlock();
}

void PipelineLoop::process_task_(PipelineTask& task, bool notify) {
    IPipelineTaskCompleter* completer = task.completer_;

    task.success_ = process_task_imp(task);
    task.state_ = PipelineTask::StateFinished;

    if (completer) {
        completer->pipeline_task_completed(task);
    } else if (notify) {
        task.sem_->post();
    }
}

status::StatusCode
PipelineLoop::process_next_subframe_(audio::Frame& frame,
                                     packet::stream_timestamp_t* frame_pos,
                                     packet::stream_timestamp_t frame_duration,
                                     audio::FrameReadMode frame_mode) {
    const size_t subframe_duration = max_samples_between_tasks_
        ? std::min(frame_duration - *frame_pos, max_samples_between_tasks_)
        : frame_duration;

    const status::StatusCode code = subframe_duration == frame_duration
        // Happy path: subframe = whole frame.
        ? process_subframe_imp(frame, frame_duration, frame_mode)
        // Heavy path: subsequently process parts of frame (sub-frames), to
        // allow processing tasks in-between.
        : make_and_process_subframe_(frame, frame_duration, *frame_pos, subframe_duration,
                                     frame_mode);

    *frame_pos += subframe_duration;

    subframe_tasks_deadline_ = timestamp_imp() + config_.max_inframe_task_processing;

    if (!enough_samples_to_process_tasks_) {
        samples_processed_ += subframe_duration;

        if (samples_processed_ >= min_samples_between_tasks_) {
            enough_samples_to_process_tasks_ = true;
        }
    }

    return code;
}

status::StatusCode
PipelineLoop::make_and_process_subframe_(audio::Frame& frame,
                                         packet::stream_timestamp_t frame_duration,
                                         packet::stream_timestamp_t subframe_pos,
                                         packet::stream_timestamp_t subframe_duration,
                                         audio::FrameReadMode subframe_mode) {
    if (direction_ == Dir_ReadFrames && subframe_pos == 0) {
        // Allocate buffer for outer frame if there is no suitable pre-allocated buffer.
        if (!frame_factory_.reallocate_frame(frame, frame_duration)) {
            return status::StatusNoMem;
        }
    }

    // Allocate subframe if not allocated yet.
    if (!subframe_ && !(subframe_ = frame_factory_.allocate_frame_no_buffer())) {
        return status::StatusNoMem;
    }

    // Attach part of outer frame's buffer to sub-frame.
    // If we're writing, frame always has a buffer.
    // If we're reading, frame either had a pre-allocated buffer provided by caller,
    // or we have (re)allocated it above.
    core::Slice<uint8_t> subframe_buffer = frame.buffer();

    const size_t byte_offset = sample_spec_.stream_timestamp_2_bytes(subframe_pos);
    const size_t byte_size = sample_spec_.stream_timestamp_2_bytes(subframe_duration);

    subframe_buffer.reslice(byte_offset, byte_offset + byte_size);
    subframe_->set_buffer(subframe_buffer);

    if (direction_ == Dir_WriteFrames) {
        // Propagate meta-data of outer frame to sub-frame.
        subframe_->set_raw(frame.is_raw());
        subframe_->set_flags(frame.flags());
        subframe_->set_duration(subframe_duration);

        if (frame.capture_timestamp()) {
            subframe_->set_capture_timestamp(
                frame.capture_timestamp()
                + sample_spec_.stream_timestamp_2_ns(subframe_pos));
        }
    }

    // Perform read or write.
    const status::StatusCode code =
        process_subframe_imp(*subframe_, subframe_duration, subframe_mode);

    if (direction_ == Dir_ReadFrames && code == status::StatusOK) {
        // Propagate meta-data and data of sub-frame to outer frame.
        frame.set_raw(subframe_->is_raw());
        frame.set_flags(frame.flags() | subframe_->flags());

        frame.set_duration(subframe_pos + subframe_duration);
        frame.set_num_bytes(
            sample_spec_.stream_timestamp_2_bytes(subframe_pos + subframe_duration));

        if (subframe_pos == 0) {
            frame.set_capture_timestamp(subframe_->capture_timestamp());
        }

        if (subframe_->buffer() != subframe_buffer) {
            // Sub-frame buffer may change because frame reader is allowed to attach
            // its own buffer instead of using pre-allocated one. In this case we
            // need to copy result back to outer frame.
            memmove(frame.bytes() + byte_offset, subframe_->bytes(), byte_size);
        }
    }

    // Clear buffer and meta-data.
    subframe_->clear();

    return code;
}

bool PipelineLoop::start_subframe_task_processing_() {
    if (pending_tasks_ == 0) {
        return false;
    }

    if (!enough_samples_to_process_tasks_) {
        return false;
    }

    enough_samples_to_process_tasks_ = false;
    samples_processed_ = 0;

    return true;
}

bool PipelineLoop::subframe_task_processing_allowed_(
    core::nanoseconds_t next_frame_deadline) const {
    const core::nanoseconds_t now = timestamp_imp();

    if (now >= subframe_tasks_deadline_) {
        return false;
    }

    if (now >= (next_frame_deadline - no_task_proc_half_interval_)) {
        return false;
    }

    return true;
}

core::nanoseconds_t
PipelineLoop::update_next_frame_deadline_(core::nanoseconds_t frame_start_time,
                                          packet::stream_timestamp_t frame_duration) {
    const core::nanoseconds_t next_frame_deadline =
        frame_start_time + sample_spec_.stream_timestamp_2_ns(frame_duration);

    next_frame_deadline_.exclusive_store(next_frame_deadline);

    return next_frame_deadline;
}

bool PipelineLoop::interframe_task_processing_allowed_(
    core::nanoseconds_t next_frame_deadline) const {
    if (!config_.enable_precise_task_scheduling) {
        // task scheduling disabled, so we just process all task in-place
        return true;
    }

    uint64_t frame_tid = 0;
    if (frame_processing_tid_.try_load(frame_tid)) {
        if (frame_tid == 0) {
            // no frames were ever processed yet
            // until the very first frame, we allow processing all tasks in-place
            return true;
        }
        if (frame_tid == tid_imp()) {
            // last frame was processed at current thread
            // we assume that frames are usually processed at the same thread, and
            // hence allow processing tasks in-place on that thread, because likely
            // it will anyway wait for task completion before proceeding to frame
            return true;
        }
    }

    // this task is scheduled not from the thread that processes frames
    // if there is enough time until next frame, we allow processing task in-place,
    // otherwise the task should be queued to avoid blocking frame processing
    const core::nanoseconds_t now = timestamp_imp();

    return now < (next_frame_deadline - no_task_proc_half_interval_)
        || now >= (next_frame_deadline + no_task_proc_half_interval_);
}

void PipelineLoop::report_stats_() {
    if (!rate_limiter_.would_allow()) {
        return;
    }

    if (!scheduler_mutex_.try_lock()) {
        return;
    }

    if (rate_limiter_.allow()) {
        roc_log(LogDebug,
                "pipeline loop:"
                " tasks=%lu in_place=%.2f in_frame=%.2f preempts=%lu sched=%lu/%lu",
                (unsigned long)stats_.task_processed_total,
                stats_.task_processed_total
                    ? double(stats_.task_processed_in_place) / stats_.task_processed_total
                    : 0.,
                stats_.task_processed_total
                    ? double(stats_.task_processed_in_frame) / stats_.task_processed_total
                    : 0.,
                (unsigned long)stats_.preemptions, (unsigned long)stats_.scheduler_calls,
                (unsigned long)stats_.scheduler_cancellations);
    }

    scheduler_mutex_.unlock();
}

} // namespace pipeline
} // namespace roc
