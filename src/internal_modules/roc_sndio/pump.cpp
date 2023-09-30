/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/pump.h"
#include "roc_core/log.h"

namespace roc {
namespace sndio {

Pump::Pump(core::BufferFactory<audio::sample_t>& buffer_factory,
           ISource& source,
           ISource* backup_source,
           ISink& sink,
           core::nanoseconds_t frame_length,
           const audio::SampleSpec& sample_spec,
           Mode mode)
    : main_source_(source)
    , backup_source_(backup_source)
    , sink_(sink)
    , sample_spec_(sample_spec)
    , n_bufs_(0)
    , oneshot_(mode == ModeOneshot)
    , stop_(0) {
    size_t frame_size = sample_spec_.ns_2_samples_overall(frame_length);
    if (frame_size == 0) {
        roc_log(LogError, "pump: frame size cannot be 0");
        return;
    }

    if (buffer_factory.buffer_size() < frame_size) {
        roc_log(LogError, "pump: buffer size is too small: required=%lu actual=%lu",
                (unsigned long)frame_size, (unsigned long)buffer_factory.buffer_size());
        return;
    }

    frame_buffer_ = buffer_factory.new_buffer();
    if (!frame_buffer_) {
        roc_log(LogError, "pump: can't allocate frame buffer");
        return;
    }

    frame_buffer_.reslice(0, frame_size);
}

bool Pump::is_valid() const {
    return frame_buffer_;
}

bool Pump::run() {
    roc_log(LogDebug, "pump: starting main loop");

    ISource* current_source = &main_source_;

    while (!stop_) {
        // switch between main and backup sources when necessary
        if (main_source_.state() == DeviceState_Active) {
            if (current_source == backup_source_) {
                roc_log(LogInfo, "pump: switching to main source");

                if (main_source_.resume()) {
                    current_source = &main_source_;
                    backup_source_->pause();
                } else {
                    roc_log(LogError, "pump: can't resume main source");
                }
            }
        } else {
            if (oneshot_ && n_bufs_ != 0) {
                roc_log(LogInfo, "pump: main source become inactive in oneshot mode");
                break;
            }

            if (backup_source_ && current_source != backup_source_) {
                roc_log(LogInfo, "pump: switching to backup source");

                if (backup_source_->restart()) {
                    current_source = backup_source_;
                    main_source_.pause();
                } else {
                    roc_log(LogError, "pump: can't restart backup source");
                }
            }
        }

        // read frame
        if (!transfer_frame_(*current_source)) {
            roc_log(LogDebug, "pump: got eof from source");

            if (current_source == backup_source_) {
                current_source = &main_source_;
                continue;
            } else {
                break;
            }
        }

        if (current_source == &main_source_) {
            n_bufs_++;
        }
    }

    roc_log(LogDebug, "pump: exiting main loop, wrote %lu buffers from main source",
            (unsigned long)n_bufs_);

    return !stop_;
}

bool Pump::transfer_frame_(ISource& current_source) {
    audio::Frame frame(frame_buffer_.data(), frame_buffer_.size());

    // if source has clock, here we block on it
    if (!current_source.read(frame)) {
        return false;
    }

    if (frame.capture_timestamp() == 0) {
        // if source does not provide capture timestamps, we fill them here
        // we subtract source latency to take into account recording buffer size,
        // where this frame spent some time before we read it
        // we subtract frame size because we already read the whole frame from
        // recording buffer, and should take it into account too
        core::nanoseconds_t capture_latency = 0;

        if (current_source.has_latency()) {
            capture_latency = current_source.latency()
                + sample_spec_.samples_overall_2_ns(frame.num_samples());
        }

        frame.set_capture_timestamp(core::timestamp(core::ClockUnix) - capture_latency);
    }

    // if sink has clock, here we block on it
    // note that either source or sink has clock, but not both
    sink_.write(frame);

    {
        // tell source what is playback time of first sample of last read frame
        // we add sink latency to take into account playback buffer size
        // we subtract frame size because we already wrote the whole frame into
        // playback buffer, and should take it into account too
        core::nanoseconds_t playback_latency = 0;

        if (sink_.has_latency()) {
            playback_latency =
                sink_.latency() - sample_spec_.samples_overall_2_ns(frame.num_samples());
        }

        current_source.reclock(core::timestamp(core::ClockUnix) + playback_latency);
    }

    return true;
}

void Pump::stop() {
    stop_ = 1;
}

} // namespace sndio
} // namespace roc
