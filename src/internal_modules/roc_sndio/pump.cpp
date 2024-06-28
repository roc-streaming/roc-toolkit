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

Pump::Pump(core::IPool& frame_pool,
           core::IPool& frame_buffer_pool,
           ISource& source,
           ISource* backup_source,
           ISink& sink,
           const Config& config,
           const Mode mode)
    : frame_factory_(frame_pool, frame_buffer_pool)
    , main_source_(source)
    , backup_source_(backup_source)
    , current_source_(&source)
    , sink_(sink)
    , sample_spec_(config.sample_spec)
    , frame_size_(config.sample_spec.ns_2_bytes(config.frame_length))
    , frame_duration_(config.sample_spec.ns_2_stream_timestamp(config.frame_length))
    , mode_(mode)
    , was_active_(false)
    , stop_(0)
    , init_status_(status::NoStatus) {
    if (frame_size_ == 0 || frame_duration_ == 0) {
        roc_log(LogError, "pump: invalid frame length %lld",
                (long long)config.frame_length);
        init_status_ = status::StatusBadConfig;
        return;
    }

    frame_ = frame_factory_.allocate_frame(frame_size_);
    if (!frame_) {
        roc_log(LogError, "pump: can't allocate frame");
        init_status_ = status::StatusNoMem;
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode Pump::init_status() const {
    return init_status_;
}

status::StatusCode Pump::run() {
    roc_log(LogDebug, "pump: starting main loop");

    status::StatusCode code = status::NoStatus;

    for (;;) {
        // Transfer one frame from source to sink.
        if ((code = next_()) != status::StatusOK) {
            break;
        }
    }

    roc_panic_if_msg(code <= status::NoStatus || code >= status::MaxStatus,
                     "pump: invalid status code %d", code);

    if (code == status::StatusEnd) {
        code = status::StatusOK; // EOF is fine
    }

    roc_log(LogDebug, "pump: exiting main loop");

    return code;
}

void Pump::stop() {
    stop_ = 1;
}

status::StatusCode Pump::next_() {
    status::StatusCode code = status::NoStatus;

    // User called stop().
    if (stop_) {
        roc_log(LogDebug, "pump: got stop request, exiting");
        return status::StatusAbort;
    }

    // Main source became inactive.
    if (current_source_ == &main_source_ && main_source_.state() == DeviceState_Idle) {
        // User specified --oneshot, so when main source becomes active and then
        // inactive first time, we exit.
        if (mode_ == ModeOneshot && was_active_) {
            roc_log(LogInfo,
                    "pump: main source became inactive in oneshot mode, exiting");
            return status::StatusEnd;
        }

        // User specified --backup, when main source becomes inactive, we
        // switch to specified backup source.
        if (backup_source_) {
            roc_log(LogInfo, "pump: main source became inactive, switching to backup");

            if ((code = backup_source_->rewind()) != status::StatusOK) {
                roc_log(LogError, "pump: can't rewind backup source: status=%s",
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
        roc_log(LogInfo, "pump: main source became active, switching to it");

        if ((code = switch_source_(&main_source_)) != status::StatusOK) {
            return code;
        }
    }

    // Transfer one frame.
    code = transfer_frame_(*current_source_, sink_);

    if (code == status::StatusEnd) {
        // EOF from main source causes exit.
        if (current_source_ == &main_source_) {
            roc_log(LogInfo, "pump: got eof from main source, exiting");
            return code;
        }

        // EOF from backup source causes rewind.
        if (current_source_ == backup_source_) {
            roc_log(LogDebug, "pump: got eof from backup source, rewinding");

            if ((code = backup_source_->rewind()) != status::StatusOK) {
                roc_log(LogError, "pump: can't rewind backup source: status=%s",
                        status::code_to_str(code));
                return code;
            }
        }
    } else if (code != status::StatusOK) {
        // Source or sink failure.
        roc_log(LogError, "pump: got error when copying frame: status=%s",
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

status::StatusCode Pump::switch_source_(ISource* new_source) {
    status::StatusCode code = status::NoStatus;

    // Switch from backup to main.
    if (new_source == &main_source_ && current_source_ != &main_source_) {
        roc_log(LogInfo, "pump: switching to main source");

        // Pause backup.
        if (backup_source_ && backup_source_->has_state()) {
            if ((code = backup_source_->pause()) != status::StatusOK) {
                roc_log(LogError, "pump: can't pause backup source: status=%s",
                        status::code_to_str(code));
                return code;
            }
        }

        // Resume main.
        if ((code = main_source_.resume()) != status::StatusOK) {
            roc_log(LogError, "pump: can't resume main source: status=%s",
                    status::code_to_str(code));
            return code;
        }

        current_source_ = &main_source_;
    }

    // Switch from main to backup.
    if (new_source == backup_source_ && current_source_ != backup_source_) {
        roc_log(LogInfo, "pump: switching to backup source");

        roc_panic_if(!backup_source_);

        // Pause main.
        if ((code = main_source_.pause()) != status::StatusOK) {
            roc_log(LogError, "pump: can't pause main source: status=%s",
                    status::code_to_str(code));
            return code;
        }

        // Resume backup.
        if (backup_source_->has_state()) {
            if ((code = backup_source_->resume()) != status::StatusOK) {
                roc_log(LogError, "pump: can't resume backup source: status=%s",
                        status::code_to_str(code));
                return code;
            }
        }

        current_source_ = backup_source_;
    }

    return status::StatusOK;
}

status::StatusCode Pump::transfer_frame_(ISource& source, ISink& sink) {
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

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
