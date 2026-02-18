/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/io_pump.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

namespace {

const core::nanoseconds_t DefaultFrameLength = 10 * core::Millisecond;

} // namespace

IoPump::IoPump(core::IPool& frame_pool,
               core::IPool& frame_buffer_pool,
               ISource& source,
               ISource* backup_source,
               ISink& sink,
               const IoConfig& io_config,
               const Mode mode)
    : frame_factory_(frame_pool, frame_buffer_pool)
    , main_source_(source)
    , backup_source_(backup_source)
    , current_source_(&source)
    , sink_(sink)
    , sample_spec_(io_config.sample_spec)
    , frame_size_(0)
    , frame_duration_(0)
    , mode_(mode)
    , was_active_(false)
    , stop_(0)
    , transferred_bytes_(0)
    , init_status_(status::NoStatus) {
    if (!io_config.sample_spec.is_complete() || !io_config.sample_spec.is_pcm()) {
        roc_panic("io pump: required complete sample spec with pcm format: spec=%s",
                  audio::sample_spec_to_str(io_config.sample_spec).c_str());
    }

    core::nanoseconds_t frame_len = io_config.frame_length;
    if (frame_len == 0) {
        frame_len = DefaultFrameLength;
    }

    frame_size_ = io_config.sample_spec.ns_2_bytes(frame_len);
    frame_duration_ = io_config.sample_spec.ns_2_stream_timestamp(frame_len);

    frame_ = frame_factory_.allocate_frame(frame_size_);
    if (!frame_) {
        roc_log(LogError, "io pump: can't allocate frame");
        init_status_ = status::StatusNoMem;
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode IoPump::init_status() const {
    return init_status_;
}

status::StatusCode IoPump::run() {
    roc_log(LogDebug, "io pump: starting main loop");

    status::StatusCode code = status::NoStatus;

    for (;;) {
        // Transfer one frame from source to sink.
        if ((code = next_()) != status::StatusOK) {
            break;
        }
    }

    if (code == status::StatusFinish) {
        // EOF is fine
        code = status::StatusOK;
        roc_log(LogDebug, "io pump: transferred %.3f MB",
                (double)transferred_bytes_ / 1024 / 1024);
    }

    if (code == status::StatusOK) {
        code = flush_sink_();
    }

    const status::StatusCode close_code = close_all_devices_();
    if (code == status::StatusOK) {
        code = close_code;
    }

    roc_log(LogDebug, "io pump: exiting main loop");

    roc_panic_if_msg(code <= status::NoStatus || code >= status::MaxStatus,
                     "io pump: invalid status code %d", code);

    return code;
}

void IoPump::stop() {
    stop_ = 1;
}

status::StatusCode IoPump::next_() {
    status::StatusCode code = status::NoStatus;

    // User called stop().
    if (stop_) {
        roc_log(LogDebug, "io pump: got stop request, exiting");
        return status::StatusAbort;
    }

    // Main source became inactive.
    if (current_source_ == &main_source_ && main_source_.state() == DeviceState_Idle) {
        // User specified --oneshot, so when main source becomes active and then
        // inactive first time, we exit.
        if (mode_ == ModeOneshot && was_active_) {
            roc_log(LogInfo,
                    "io pump: main source became inactive in oneshot mode, exiting");
            return status::StatusFinish;
        }

        // User specified --backup, when main source becomes inactive, we
        // switch to specified backup source.
        if (backup_source_) {
            roc_log(LogInfo, "io pump: main source became inactive, switching to backup");

            if ((code = backup_source_->rewind()) != status::StatusOK) {
                roc_log(LogError, "io pump: can't rewind backup source: status=%s",
                        status::code_to_str(code));
                return code;
            }

            if ((code = switch_source_(backup_source_)) != status::StatusOK) {
                return code;
            }
        }
    }

    // Main source became active.
    if (current_source_ != &main_source_ && main_source_.state() == DeviceState_Active) {
        roc_log(LogInfo, "io pump: main source became active, switching to it");

        if ((code = switch_source_(&main_source_)) != status::StatusOK) {
            return code;
        }
    }

    // Transfer one frame.
    code = transfer_frame_(*current_source_, sink_);

    if (code == status::StatusFinish) {
        // EOF from main source causes exit.
        if (current_source_ == &main_source_) {
            roc_log(LogInfo, "io pump: got eof from main source, exiting");
            return code;
        }

        // EOF from backup source causes rewind.
        if (current_source_ == backup_source_) {
            roc_log(LogDebug, "io pump: got eof from backup source, rewinding");

            if ((code = backup_source_->rewind()) != status::StatusOK) {
                roc_log(LogError, "io pump: can't rewind backup source: status=%s",
                        status::code_to_str(code));
                return code;
            }
        }
    } else if (code != status::StatusOK) {
        // Source or sink failure.
        roc_log(LogError, "io pump: got error when copying frame: status=%s",
                status::code_to_str(code));
        return code;
    }

    if (current_source_ == &main_source_
        && current_source_->state() == DeviceState_Active) {
        // Remember that main source was active and we've read something.
        was_active_ = true;
    }

    return status::StatusOK;
}

status::StatusCode IoPump::switch_source_(ISource* new_source) {
    status::StatusCode code = status::NoStatus;

    // Switch from backup to main.
    if (new_source == &main_source_ && current_source_ != &main_source_) {
        roc_log(LogInfo, "io pump: switching to main source");

        // Pause backup.
        if (backup_source_ && backup_source_->has_state()) {
            if ((code = backup_source_->pause()) != status::StatusOK) {
                roc_log(LogError, "io pump: can't pause backup source: status=%s",
                        status::code_to_str(code));
                return code;
            }
        }

        // Resume main.
        if ((code = main_source_.resume()) != status::StatusOK) {
            roc_log(LogError, "io pump: can't resume main source: status=%s",
                    status::code_to_str(code));
            return code;
        }

        current_source_ = &main_source_;
    }

    // Switch from main to backup.
    if (new_source == backup_source_ && current_source_ != backup_source_) {
        roc_log(LogInfo, "io pump: switching to backup source");

        roc_panic_if(!backup_source_);

        // Pause main.
        if ((code = main_source_.pause()) != status::StatusOK) {
            roc_log(LogError, "io pump: can't pause main source: status=%s",
                    status::code_to_str(code));
            return code;
        }

        // Resume backup.
        if (backup_source_->has_state()) {
            if ((code = backup_source_->resume()) != status::StatusOK) {
                roc_log(LogError, "io pump: can't resume backup source: status=%s",
                        status::code_to_str(code));
                return code;
            }
        }

        current_source_ = backup_source_;
    }

    return status::StatusOK;
}

status::StatusCode IoPump::transfer_frame_(ISource& source, ISink& sink) {
    // If writer stole frame's buffer, allocate it again.
    if (!frame_factory_.reallocate_frame(*frame_, frame_size_)) {
        return status::StatusNoMem;
    }

    status::StatusCode frame_status = status::NoStatus;

    // Fill frame from source.
    // If source has clock, here we block on it.
    frame_status = source.read(*frame_, frame_duration_, audio::ModeHard);
    if (frame_status != status::StatusOK && frame_status != status::StatusPart) {
        return frame_status;
    }

    if (frame_->capture_timestamp() == 0) {
        // If source does not provide capture timestamps, we fill them here.
        // We subtract source latency to take into account recording buffer size,
        // where this frame spent some time before we read it.
        // We subtract frame size because we already read the whole frame from
        // recording buffer, and should take it into account too.
        core::nanoseconds_t capture_latency = 0;

        if (source.has_latency()) {
            capture_latency =
                source.latency() + sample_spec_.stream_timestamp_2_ns(frame_->duration());
        }

        frame_->set_capture_timestamp(core::timestamp(core::ClockUnix) - capture_latency);
    }

    // Pass frame to sink.
    // If sink has clock, here we block on it.
    // Note that either source or sink can have clock, but not both.
    frame_status = sink.write(*frame_);
    if (frame_status != status::StatusOK) {
        return frame_status;
    }

    {
        // Tell source what is playback time of first sample of last read frame.
        // We add sink latency to take into account playback buffer size.
        // We subtract frame size because we already wrote the whole frame into
        // playback buffer, and should take it into account too.
        core::nanoseconds_t playback_latency = 0;

        if (sink_.has_latency()) {
            playback_latency =
                sink_.latency() - sample_spec_.stream_timestamp_2_ns(frame_->duration());
        }

        source.reclock(core::timestamp(core::ClockUnix) + playback_latency);
    }

    transferred_bytes_ += frame_->num_bytes();

    return status::StatusOK;
}

status::StatusCode IoPump::flush_sink_() {
    const status::StatusCode code = sink_.flush();

    if (code != status::StatusOK) {
        roc_log(LogError, "io pump: got error when flushing sink: status=%s",
                status::code_to_str(code));
    }

    return code;
}

status::StatusCode IoPump::close_all_devices_() {
    IDevice* devices[] = { &main_source_, &sink_, backup_source_ };
    status::StatusCode first_error = status::StatusOK;

    for (size_t i = 0; i < ROC_ARRAY_SIZE(devices); ++i) {
        if (devices[i]) {
            status::StatusCode device_code = devices[i]->close();
            if (device_code != status::StatusOK) {
                roc_log(LogError, "io pump: failed to close device: status=%s",
                        status::code_to_str(device_code));
                if (first_error == status::StatusOK) {
                    first_error = device_code;
                }
            }
        }
    }

    return first_error;
}

} // namespace sndio
} // namespace roc
