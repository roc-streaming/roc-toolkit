/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/task_pipeline.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace pipeline {

TaskPipeline::Task::Task()
    : state_(StateNew)
    , success_(false)
    , handler_(NULL) {
}

TaskPipeline::Task::~Task() {
    if (state_ == StateScheduled) {
        roc_panic("task pipeline: attempt to destroy task before it's finished");
    }
}

bool TaskPipeline::Task::success() const {
    return state_ == StateFinished && success_;
}

TaskPipeline::ICompletionHandler::~ICompletionHandler() {
}

TaskPipeline::TaskPipeline(ITaskScheduler& scheduler,
                           const TaskConfig& config,
                           size_t sample_rate,
                           packet::channel_mask_t ch_mask)
    : config_(config)
    , sample_rate_(sample_rate)
    , ch_mask_(ch_mask)
    , min_samples_between_tasks_(
          packet::ns_to_size(config.min_frame_length_between_tasks, sample_rate, ch_mask))
    , max_samples_between_tasks_(
          packet::ns_to_size(config.max_frame_length_between_tasks, sample_rate, ch_mask))
    , scheduler_(scheduler)
    , task_finished_(task_queue_mutex_)
    , pending_tasks_(0)
    , subframe_tasks_deadline_(0)
    , curr_frame_tasks_deadline_(0)
    , next_frame_tasks_deadline_(0)
    , samples_processed_(0)
    , enough_samples_to_process_tasks_(false) {
}

TaskPipeline::~TaskPipeline() {
    core::Mutex::Lock lock(task_queue_mutex_);

    if (pending_tasks_ != 0) {
        roc_panic(
            "task pipeline: attempt to destroy pipeline before finishing all tasks");
    }
}

TaskPipeline::Stats TaskPipeline::get_stats() const {
    core::Mutex::Lock lock(task_queue_mutex_);

    return stats_;
}

size_t TaskPipeline::num_pending_tasks() const {
    core::Mutex::Lock lock(task_queue_mutex_);

    return (size_t)pending_tasks_;
}

size_t TaskPipeline::num_pending_frames() const {
    return (size_t)pending_frames_;
}

void TaskPipeline::schedule(Task& task, ICompletionHandler& handler) {
    task.handler_ = &handler;

    schedule_task_(task, false);
}

bool TaskPipeline::schedule_and_wait(Task& task) {
    schedule_task_(task, true);

    return task.success_;
}

void TaskPipeline::schedule_task_(Task& task, bool wait_finished) {
    task_queue_mutex_.lock();

    if (task.state_ != Task::StateNew) {
        roc_panic("task pipeline: attempt to schedule task more than once");
    }

    task.state_ = Task::StateScheduled;
    pending_tasks_++;

    if (pending_tasks_ == 1 && interframe_task_processing_allowed_()
        && pipeline_mutex_.try_lock()) {
        task_queue_mutex_.unlock();
        process_task_(task);
        task_queue_mutex_.lock();

        pending_tasks_--;

        stats_.task_processed_total++;
        stats_.task_processed_in_place++;

        bool pending_frame = pending_frames_;
        if (pending_frame) {
            stats_.preemptions++;
        }

        if (task_queue_.size() != 0 && !pending_frame) {
            schedule_async_task_processing_();
        }

        pipeline_mutex_.unlock();
    } else {
        task_queue_.push_back(task);

        if (pipeline_mutex_.try_lock()) {
            schedule_async_task_processing_();
            pipeline_mutex_.unlock();
        }
    }

    if (wait_finished) {
        while (task.state_ != Task::StateFinished) {
            task_finished_.wait();
        }
    }

    task_queue_mutex_.unlock();
}

void TaskPipeline::process_tasks() {
    task_queue_mutex_.lock();

    processing_scheduled_ = false;

    if (pipeline_mutex_.try_lock()) {
        bool pending_frame = false;

        while (task_queue_.size() != 0 && interframe_task_processing_allowed_()
               && !(pending_frame = pending_frames_)) {
            Task* task = task_queue_.front();
            task_queue_.remove(*task);

            task_queue_mutex_.unlock();
            process_task_(*task);
            task_queue_mutex_.lock();

            pending_tasks_--;

            stats_.task_processed_total++;
        }

        if (pending_frame) {
            stats_.preemptions++;
        }

        if (task_queue_.size() != 0 && !pending_frame) {
            schedule_async_task_processing_();
        }

        pipeline_mutex_.unlock();
    }

    task_queue_mutex_.unlock();
}

bool TaskPipeline::process_frame_and_tasks(audio::Frame& frame) {
    if (config_.enable_precise_task_scheduling) {
        return process_frame_and_tasks_precise_(frame);
    }
    return process_frame_and_tasks_simple_(frame);
}

bool TaskPipeline::process_frame_and_tasks_simple_(audio::Frame& frame) {
    ++pending_frames_;

    pipeline_mutex_.lock();

    cancel_async_task_processing_();

    const bool frame_res = process_frame_imp(frame);

    task_queue_mutex_.lock();

    if (task_queue_.size() != 0) {
        schedule_async_task_processing_();
    }

    pipeline_mutex_.unlock();

    --pending_frames_;

    task_queue_mutex_.unlock();

    return frame_res;
}

bool TaskPipeline::process_frame_and_tasks_precise_(audio::Frame& frame) {
    ++pending_frames_;

    const core::nanoseconds_t frame_start_time = timestamp_imp();

    pipeline_mutex_.lock();

    cancel_async_task_processing_();

    size_t frame_pos = 0;
    bool frame_res = false;

    for (;;) {
        const bool first_iteration = (frame_pos == 0);

        frame_res = process_next_subframe_(frame, &frame_pos);

        task_queue_mutex_.lock();

        if (first_iteration) {
            update_interframe_deadlines_(frame_start_time, frame.size());
        }

        if (start_subframe_task_processing_()) {
            while (Task* task = task_queue_.front()) {
                task_queue_.remove(*task);

                task_queue_mutex_.unlock();
                process_task_(*task);
                task_queue_mutex_.lock();

                pending_tasks_--;

                stats_.task_processed_total++;
                stats_.task_processed_in_frame++;

                if (!subframe_task_processing_allowed_()
                    || !interframe_task_processing_allowed_()) {
                    break;
                }
            }
        }

        if (!frame_res || frame_pos == frame.size()) {
            break;
        }

        task_queue_mutex_.unlock();
    }

    if (task_queue_.size() != 0) {
        schedule_async_task_processing_();
    }

    pipeline_mutex_.unlock();

    --pending_frames_;

    task_queue_mutex_.unlock();

    return frame_res;
}

void TaskPipeline::schedule_async_task_processing_() {
    if (processing_scheduled_) {
        return;
    }
    processing_scheduled_ = true;

    core::nanoseconds_t delay = 0;

    if (config_.enable_precise_task_scheduling) {
        const core::nanoseconds_t now = timestamp_imp();

        if (now < curr_frame_tasks_deadline_) {
            delay = 0;
        } else if (now < next_frame_tasks_deadline_) {
            delay = (next_frame_tasks_deadline_ - now);
        } else {
            delay = 0;
        }
    }

    scheduler_.schedule_task_processing(*this, delay);

    stats_.scheduler_calls++;
}

void TaskPipeline::cancel_async_task_processing_() {
    if (!processing_scheduled_) {
        return;
    }
    processing_scheduled_ = false;

    scheduler_.cancel_task_processing(*this);

    stats_.scheduler_cancellations++;
}

void TaskPipeline::process_task_(Task& task) {
    ICompletionHandler* handler = task.handler_;

    task.success_ = process_task_imp(task);
    task.state_ = Task::StateFinished;

    if (handler) {
        handler->pipeline_task_finished(task);
    } else {
        task_finished_.broadcast();
    }
}

bool TaskPipeline::process_next_subframe_(audio::Frame& frame, size_t* frame_pos) {
    const size_t subframe_size = max_samples_between_tasks_
        ? std::min(frame.size() - *frame_pos, max_samples_between_tasks_)
        : frame.size();

    audio::Frame sub_frame(frame.data() + *frame_pos, subframe_size);

    const bool ret = process_frame_imp(sub_frame);

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

void TaskPipeline::update_interframe_deadlines_(core::nanoseconds_t frame_start_time,
                                                size_t frame_size) {
    const core::nanoseconds_t frame_duration =
        packet::size_to_ns(frame_size, sample_rate_, ch_mask_);

    curr_frame_tasks_deadline_ = frame_start_time + frame_duration
        - config_.task_processing_prohibited_interval / 2;

    next_frame_tasks_deadline_ = frame_start_time + frame_duration
        + config_.task_processing_prohibited_interval / 2;
}

bool TaskPipeline::start_subframe_task_processing_() {
    if (task_queue_.size() == 0) {
        return false;
    }

    if (!enough_samples_to_process_tasks_) {
        return false;
    }

    enough_samples_to_process_tasks_ = false;
    samples_processed_ = 0;

    return true;
}

bool TaskPipeline::interframe_task_processing_allowed_() const {
    if (!config_.enable_precise_task_scheduling) {
        return true;
    }

    const core::nanoseconds_t now = timestamp_imp();

    return now < curr_frame_tasks_deadline_ || now >= next_frame_tasks_deadline_;
}

bool TaskPipeline::subframe_task_processing_allowed_() const {
    const core::nanoseconds_t now = timestamp_imp();

    return now < subframe_tasks_deadline_;
}

} // namespace pipeline
} // namespace roc
