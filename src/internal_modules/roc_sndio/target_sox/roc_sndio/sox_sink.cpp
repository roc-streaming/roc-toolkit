/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sox_sink.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/backend_map.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

namespace {

const core::nanoseconds_t DefaultFrameLength = 10 * core::Millisecond;

} // namespace

SoxSink::SoxSink(audio::FrameFactory& frame_factory,
                 core::IArena& arena,
                 const IoConfig& io_config,
                 const char* driver,
                 const char* path)
    : IDevice(arena)
    , ISink(arena)
    , driver_(arena)
    , path_(arena)
    , output_(NULL)
    , buffer_(arena)
    , buffer_size_(0)
    , paused_(false)
    , init_status_(status::NoStatus) {
    BackendMap::instance();

    if (io_config.latency != 0) {
        roc_log(LogError, "sox sink: setting io latency not implemented for sox backend");
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (io_config.sample_spec.has_format()
        && io_config.sample_spec.format() != audio::Format_Pcm) {
        roc_log(LogError,
                "sox sink: invalid io encoding:"
                " <format> '%s' not supported by backend: spec=%s",
                io_config.sample_spec.format_name(),
                audio::sample_spec_to_str(io_config.sample_spec).c_str());
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (io_config.sample_spec.has_subformat()) {
        if (io_config.sample_spec.pcm_subformat() == audio::PcmSubformat_Invalid) {
            roc_log(LogError,
                    "sox sink: invalid io encoding:"
                    " <subformat> '%s' not supported by backend: spec=%s",
                    io_config.sample_spec.subformat_name(),
                    audio::sample_spec_to_str(io_config.sample_spec).c_str());
            init_status_ = status::StatusBadConfig;
            return;
        }

        const audio::PcmTraits subfmt =
            audio::pcm_subformat_traits(io_config.sample_spec.pcm_subformat());

        if (!subfmt.has_flags(audio::Pcm_IsInteger | audio::Pcm_IsSigned)) {
            roc_log(LogError,
                    "sox sink: invalid io encoding:"
                    " <subformat> must be signed integer (like s16): spec=%s",
                    audio::sample_spec_to_str(io_config.sample_spec).c_str());
            init_status_ = status::StatusBadConfig;
            return;
        }

        if (!subfmt.has_flags(audio::Pcm_IsPacked | audio::Pcm_IsAligned)) {
            roc_log(LogError,
                    "sox sink: invalid io encoding:"
                    " <subformat> must be packed (like s24, not s24_4) and byte-aligned"
                    " (like s16, not s18): spec=%s",
                    audio::sample_spec_to_str(io_config.sample_spec).c_str());
            init_status_ = status::StatusBadConfig;
            return;
        }

        if (io_config.sample_spec.pcm_subformat() != subfmt.default_variant) {
            roc_log(LogError,
                    "sox sink: invalid io encoding:"
                    " <subformat> must be default-endian (like s16, not s16_le): spec=%s",
                    audio::sample_spec_to_str(io_config.sample_spec).c_str());
            init_status_ = status::StatusBadConfig;
            return;
        }
    }

    out_spec_ = io_config.sample_spec;
    if (!out_spec_.has_format()) {
        out_spec_.set_format(audio::Format_Pcm);
        out_spec_.set_pcm_subformat(audio::PcmSubformat_SInt16);
    }

    frame_length_ = io_config.frame_length;
    if (frame_length_ == 0) {
        frame_length_ = DefaultFrameLength;
    }

    roc_log(LogDebug, "sox sink: opening: driver=%s path=%s", driver, path);

    if ((init_status_ = init_names_(driver, path)) != status::StatusOK) {
        return;
    }

    if ((init_status_ = open_()) != status::StatusOK) {
        return;
    }

    if ((init_status_ = init_buffer_()) != status::StatusOK) {
        return;
    }

    init_status_ = status::StatusOK;
}

SoxSink::~SoxSink() {
    const status::StatusCode code = close();
    if (code != status::StatusOK) {
        roc_log(LogError, "sox sink: close failed: status=%s", status::code_to_str(code));
    }
}

status::StatusCode SoxSink::init_status() const {
    return init_status_;
}

DeviceType SoxSink::type() const {
    return DeviceType_Sink;
}

ISink* SoxSink::to_sink() {
    return this;
}

ISource* SoxSink::to_source() {
    return NULL;
}

audio::SampleSpec SoxSink::sample_spec() const {
    return frame_spec_;
}

core::nanoseconds_t SoxSink::frame_length() const {
    return frame_length_;
}

bool SoxSink::has_state() const {
    return true;
}

DeviceState SoxSink::state() const {
    if (paused_) {
        return DeviceState_Paused;
    } else {
        return DeviceState_Active;
    }
}

status::StatusCode SoxSink::pause() {
    if (paused_) {
        return status::StatusOK;
    }

    if (!output_) {
        roc_panic("sox sink: not opened");
    }

    roc_log(LogDebug, "sox sink: pausing: driver=%s path=%s", driver_.c_str(),
            path_.c_str());

    const status::StatusCode close_code = close_();
    if (close_code != status::StatusOK) {
        return close_code;
    }

    paused_ = true;

    return status::StatusOK;
}

status::StatusCode SoxSink::resume() {
    if (!paused_) {
        return status::StatusOK;
    }

    roc_log(LogDebug, "sox sink: resuming: driver=%s path=%s", driver_.c_str(),
            path_.c_str());

    if (!output_) {
        const status::StatusCode code = open_();
        if (code != status::StatusOK) {
            return code;
        }
    }

    paused_ = false;

    return status::StatusOK;
}

bool SoxSink::has_latency() const {
    return false;
}

bool SoxSink::has_clock() const {
    return true;
}

status::StatusCode SoxSink::write(audio::Frame& frame) {
    frame_spec_.validate_frame(frame);

    const audio::sample_t* frame_data = frame.raw_samples();
    size_t frame_size = frame.num_raw_samples();

    sox_sample_t* buffer_data = buffer_.data();
    size_t buffer_pos = 0;

    SOX_SAMPLE_LOCALS;

    size_t clips = 0;

    while (frame_size > 0) {
        for (; buffer_pos < buffer_size_ && frame_size > 0; buffer_pos++) {
            buffer_data[buffer_pos] = SOX_FLOAT_32BIT_TO_SAMPLE(*frame_data, clips);
            frame_data++;
            frame_size--;
        }

        if (buffer_pos == buffer_size_) {
            if (sox_write(output_, buffer_data, buffer_pos) != buffer_pos) {
                roc_log(LogError, "sox sink: failed to write output buffer");
                return status::StatusErrDevice;
            }
            buffer_pos = 0;
        }
    }

    if (buffer_pos > 0) {
        if (sox_write(output_, buffer_data, buffer_pos) != buffer_pos) {
            roc_log(LogError, "sox sink: failed to write output buffer");
            return status::StatusErrDevice;
        }
    }

    return status::StatusOK;
}

status::StatusCode SoxSink::flush() {
    return status::StatusOK;
}

status::StatusCode SoxSink::close() {
    return close_();
}

void SoxSink::dispose() {
    arena().dispose_object(*this);
}

status::StatusCode SoxSink::init_names_(const char* driver, const char* path) {
    if (driver) {
        if (!driver_.assign(driver)) {
            roc_log(LogError, "sox sink: can't allocate string");
            return status::StatusNoMem;
        }
    }

    if (path) {
        if (!path_.assign(path)) {
            roc_log(LogError, "sox sink: can't allocate string");
            return status::StatusNoMem;
        }
    }

    return status::StatusOK;
}

status::StatusCode SoxSink::init_buffer_() {
    buffer_size_ = frame_spec_.ns_2_samples_overall(frame_length_);
    if (buffer_size_ == 0) {
        roc_log(LogError, "sox sink: buffer size is zero");
        return status::StatusBadConfig;
    }
    if (!buffer_.resize(buffer_size_)) {
        roc_log(LogError, "sox sink: can't allocate sample buffer");
        return status::StatusNoMem;
    }

    return status::StatusOK;
}

status::StatusCode SoxSink::open_() {
    memset(&out_signal_, 0, sizeof(out_signal_));
    out_signal_.rate = (sox_rate_t)out_spec_.sample_rate();
    out_signal_.channels = (unsigned)out_spec_.num_channels();
    out_signal_.precision = (unsigned)out_spec_.pcm_bit_width();

    output_ = sox_open_write(path_.is_empty() ? NULL : path_.c_str(), &out_signal_, NULL,
                             driver_.is_empty() ? NULL : driver_.c_str(), NULL, NULL);
    if (!output_) {
        roc_log(LogDebug, "sox sink: can't open: driver=%s path=%s", driver_.c_str(),
                path_.c_str());
        return status::StatusErrDevice;
    }

    const unsigned long requested_rate = (unsigned long)out_signal_.rate;
    const unsigned long actual_rate = (unsigned long)output_->signal.rate;

    if (requested_rate != 0 && requested_rate != actual_rate) {
        roc_log(LogError,
                "sox sink:"
                " can't open output device with the requested sample rate:"
                " supported=%lu requested=%lu",
                actual_rate, requested_rate);
        return status::StatusErrDevice;
    }

    const unsigned long requested_chans = (unsigned long)out_signal_.channels;
    const unsigned long actual_chans = (unsigned long)output_->signal.channels;

    if (requested_chans != 0 && requested_chans != actual_chans) {
        roc_log(LogError,
                "sox sink:"
                " can't open output device with the requested channel count:"
                " supported=%lu requested=%lu",
                actual_chans, requested_chans);
        return status::StatusErrDevice;
    }

    const unsigned long requested_bits = (unsigned long)out_signal_.precision;
    const unsigned long actual_bits = (unsigned long)output_->signal.precision;

    if (requested_bits != 0 && requested_bits != actual_bits) {
        roc_log(LogError,
                "sox sink:"
                " can't open output device with the requested subformat:"
                " supported=s%lu requested=s%lu",
                actual_bits, requested_bits);
        return status::StatusErrDevice;
    }

    out_spec_.set_sample_rate(actual_rate);
    out_spec_.channel_set().set_layout(audio::ChanLayout_Surround);
    out_spec_.channel_set().set_order(audio::ChanOrder_Smpte);
    out_spec_.channel_set().set_count(actual_chans);

    frame_spec_ = out_spec_;
    frame_spec_.set_format(audio::Format_Pcm);
    frame_spec_.set_pcm_subformat(audio::PcmSubformat_Raw);

    roc_log(LogInfo, "sox sink: opened output device: %s",
            audio::sample_spec_to_str(out_spec_).c_str());

    return status::StatusOK;
}

status::StatusCode SoxSink::close_() {
    if (!output_) {
        return status::StatusOK;
    }

    roc_log(LogInfo, "sox sink: closing output");

    const int err = sox_close(output_);
    output_ = NULL;

    if (err != SOX_SUCCESS) {
        roc_log(LogError, "sox sink: can't close output: %s", sox_strerror(err));
        return status::StatusErrDevice;
    }

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
