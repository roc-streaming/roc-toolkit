/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/pump.h"
#include "roc_core/log.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

Pump::Pump(core::IPool& buffer_pool,
           ISource& source,
           ISource* backup_source,
           ISink& sink,
           core::nanoseconds_t frame_length,
           const audio::SampleSpec& sample_spec,
           Mode mode)
    : frame_factory_(buffer_pool)
    , main_source_(source)
    , backup_source_(backup_source)
    , sink_(sink)
    , sample_spec_(sample_spec)
    , mode_(mode)
    , stop_(0)
    , init_status_(status::NoStatus) {
    size_t frame_size = sample_spec_.ns_2_samples_overall(frame_length);
    if (frame_size == 0) {
        roc_log(LogError, "pump: frame size cannot be 0");
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (frame_factory_.raw_buffer_size() < frame_size) {
        roc_log(LogError, "pump: buffer size is too small: required=%lu actual=%lu",
                (unsigned long)frame_size,
                (unsigned long)frame_factory_.raw_buffer_size());
        init_status_ = status::StatusNoSpace;
        return;
    }

    frame_buffer_ = frame_factory_.new_raw_buffer();
    if (!frame_buffer_) {
        roc_log(LogError, "pump: can't allocate frame buffer");
        init_status_ = status::StatusNoMem;
        return;
    }

    frame_buffer_.reslice(0, frame_size);

    init_status_ = status::StatusOK;
}

status::StatusCode Pump::init_status() const {
    return init_status_;
}

status::StatusCode Pump::run() {
    roc_log(LogDebug, "pump: starting main loop");

    const status::StatusCode code = transfer_loop_();

    roc_log(LogDebug, "pump: exiting main loop");

    return code;
}

void Pump::stop() {
    stop_ = 1;
}

status::StatusCode Pump::transfer_loop_() {
    ISource* current_source = &main_source_;

    bool was_active_ = false;

    for (;;) {
        if (stop_) {
            roc_log(LogDebug, "pump: got stop request, exiting");
            return status::StatusAbort;
        }

        // switch between main and backup sources when necessary
        if (main_source_.state() == DeviceState_Active) {
            if (current_source == backup_source_) {
                roc_log(LogInfo, "pump: switching to main source");

                if (main_source_.resume()) {
                    current_source = &main_source_;
                    backup_source_->pause();
                } else {
                    roc_log(LogError, "pump: can't resume main source");
                    // TODO(gh-183): forward status from resume()
                }
            }
        } else {
            if (mode_ == ModeOneshot && was_active_) {
                roc_log(LogInfo, "pump: main source became inactive, exiting");
                return status::StatusEnd;
            }

            if (backup_source_ && current_source != backup_source_) {
                roc_log(LogInfo, "pump: switching to backup source");

                if (backup_source_->restart()) {
                    current_source = backup_source_;
                    main_source_.pause();
                } else {
                    roc_log(LogError, "pump: can't restart backup source");
                    // TODO(gh-183): forward status from restart()
                }
            }
        }

        // read frame
        const status::StatusCode code = transfer_frame_(*current_source);
        if (code == status::StatusEnd) {
            if (current_source == backup_source_) {
                roc_log(LogDebug, "pump: got eof from backup source");
                current_source = &main_source_;
                continue;
            } else {
                roc_log(LogInfo, "pump: got eof from main source, exiting");
                return code;
            }
        } else if (code != status::StatusOK) {
            roc_log(LogError, "pump: got error from source: status=%s",
                    status::code_to_str(code));
            return code;
        }

        if (current_source == &main_source_) {
            was_active_ = true;
        }
    }
}

status::StatusCode Pump::transfer_frame_(ISource& current_source) {
    audio::Frame frame(frame_buffer_.data(), frame_buffer_.size());

    // if source has clock, here we block on it
    if (!current_source.read(frame)) {
        return status::StatusEnd;
    }

    if (!frame.has_duration()) {
        // if source does not provide frame duration, we fill it here
        // we assume that the frame has some PCM format
        frame.set_duration(sample_spec_.bytes_2_stream_timestamp(frame.num_bytes()));
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
                + sample_spec_.stream_timestamp_2_ns(frame.duration());
        }

        frame.set_capture_timestamp(core::timestamp(core::ClockUnix) - capture_latency);
    }

    // if sink has clock, here we block on it
    // note that either source or sink has clock, but not both
    const status::StatusCode code = sink_.write(frame);
    if (code != status::StatusOK) {
        return code;
    }

    {
        // tell source what is playback time of first sample of last read frame
        // we add sink latency to take into account playback buffer size
        // we subtract frame size because we already wrote the whole frame into
        // playback buffer, and should take it into account too
        core::nanoseconds_t playback_latency = 0;

        if (sink_.has_latency()) {
            playback_latency =
                sink_.latency() - sample_spec_.stream_timestamp_2_ns(frame.duration());
        }

        current_source.reclock(core::timestamp(core::ClockUnix) + playback_latency);
    }

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
