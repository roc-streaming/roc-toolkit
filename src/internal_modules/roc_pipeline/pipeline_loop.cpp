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
                           const audio::SampleSpec& sample_spec)
    : config_(config)
    , sample_spec_(sample_spec)
    , min_samples_between_tasks_(
          sample_spec.ns_2_samples_overall(config.min_frame_length_between_tasks))
    , max_samples_between_tasks_(
          sample_spec.ns_2_samples_overall(config.max_frame_length_between_tasks))
    , no_task_proc_half_interval_(config.task_processing_prohibited_interval / 2)
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

const PipelineLoop::Stats& PipelineLoop::get_stats_ref() const {
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

bool PipelineLoop::process_subframes_and_tasks(audio::Frame& frame) {
    if (config_.enable_precise_task_scheduling) {
        return process_subframes_and_tasks_precise_(frame);
    }
    return process_subframes_and_tasks_simple_(frame);
}

bool PipelineLoop::process_subframes_and_tasks_simple_(audio::Frame& frame) {
    ++pending_frames_;

    cancel_async_task_processing_();

    pipeline_mutex_.lock();

    const bool frame_res = process_subframe_imp(frame);

    pipeline_mutex_.unlock();

    if (--pending_frames_ == 0 && pending_tasks_ != 0) {
        schedule_async_task_processing_();
    }

    return frame_res;
}

bool PipelineLoop::process_subframes_and_tasks_precise_(audio::Frame& frame) {
    ++pending_frames_;

    const core::nanoseconds_t frame_start_time = timestamp_imp();

    cancel_async_task_processing_();

    pipeline_mutex_.lock();

    core::nanoseconds_t next_frame_deadline = 0;

    size_t frame_pos = 0;
    bool frame_res = false;

    for (;;) {
        const bool first_iteration = (frame_pos == 0);

        frame_res = process_next_subframe_(frame, &frame_pos);

        if (first_iteration) {
            next_frame_deadline =
                update_next_frame_deadline_(frame_start_time, frame.num_samples());
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

        if (!frame_res || frame_pos == frame.num_samples()) {
            break;
        }
    }

    report_stats_();

    frame_processing_tid_.exclusive_store(tid_imp());

    pipeline_mutex_.unlock();

    if (--pending_frames_ == 0 && pending_tasks_ != 0) {
        schedule_async_task_processing_();
    }

    return frame_res;
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

bool PipelineLoop::process_next_subframe_(audio::Frame& frame, size_t* frame_pos) {
    const size_t subframe_size = max_samples_between_tasks_
        ? std::min(frame.num_samples() - *frame_pos, max_samples_between_tasks_)
        : frame.num_samples();

    audio::Frame sub_frame(frame.samples() + *frame_pos, subframe_size);

    sub_frame.set_flags(frame.flags());
    sub_frame.set_capture_timestamp(
        frame.capture_timestamp()
            ? frame.capture_timestamp() + sample_spec_.samples_overall_2_ns(*frame_pos)
            : 0);

    const bool ret = process_subframe_imp(sub_frame);

    subframe_tasks_deadline_ = timestamp_imp() + config_.max_inframe_task_processing;

    *frame_pos += subframe_size;

    if (!enough_samples_to_process_tasks_) {
        samples_processed_ += subframe_size;
        if (samples_processed_ >= min_samples_between_tasks_) {
            enough_samples_to_process_tasks_ = true;
        }
    }

    return ret;
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
                                          size_t frame_size) {
    const core::nanoseconds_t frame_duration =
        sample_spec_.samples_overall_2_ns(frame_size);

    const core::nanoseconds_t next_frame_deadline = frame_start_time + frame_duration;

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
