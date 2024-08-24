/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sox_source.h"
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

SoxSource::SoxSource(audio::FrameFactory& frame_factory,
                     core::IArena& arena,
                     const IoConfig& io_config,
                     const char* driver,
                     const char* path)
    : IDevice(arena)
    , ISource(arena)
    , frame_factory_(frame_factory)
    , driver_(arena)
    , path_(arena)
    , buffer_(arena)
    , buffer_size_(0)
    , input_(NULL)
    , paused_(false)
    , init_status_(status::NoStatus) {
    BackendMap::instance();

    if (io_config.latency != 0) {
        roc_log(LogError,
                "sox source: setting io latency not implemented for sox backend");
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (io_config.sample_spec.has_format()
        && io_config.sample_spec.format() != audio::Format_Pcm) {
        roc_log(LogError,
                "sox source: invalid io encoding:"
                " <format> '%s' not supported by backend: spec=%s",
                io_config.sample_spec.format_name(),
                audio::sample_spec_to_str(io_config.sample_spec).c_str());
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (io_config.sample_spec.has_subformat()) {
        if (io_config.sample_spec.pcm_subformat() == audio::PcmSubformat_Invalid) {
            roc_log(LogError,
                    "sox source: invalid io encoding:"
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
                    "sox source: invalid io encoding:"
                    " <subformat> must be signed integer (like s16): spec=%s",
                    audio::sample_spec_to_str(io_config.sample_spec).c_str());
            init_status_ = status::StatusBadConfig;
            return;
        }

        if (!subfmt.has_flags(audio::Pcm_IsPacked | audio::Pcm_IsAligned)) {
            roc_log(LogError,
                    "sox source: invalid io encoding:"
                    " <subformat> must be packed (like s24, not s24_4) and byte-aligned"
                    " (like s16, not s18): spec=%s",
                    audio::sample_spec_to_str(io_config.sample_spec).c_str());
            init_status_ = status::StatusBadConfig;
            return;
        }

        if (io_config.sample_spec.pcm_subformat() != subfmt.default_variant) {
            roc_log(LogError,
                    "sox source: invalid io encoding:"
                    " <subformat> must be default-endian (like s16, not s16_le): spec=%s",
                    audio::sample_spec_to_str(io_config.sample_spec).c_str());
            init_status_ = status::StatusBadConfig;
            return;
        }
    }

    in_spec_ = io_config.sample_spec;
    if (!in_spec_.has_format()) {
        in_spec_.set_format(audio::Format_Pcm);
        in_spec_.set_pcm_subformat(audio::PcmSubformat_SInt16);
    }

    frame_length_ = io_config.frame_length;
    if (frame_length_ == 0) {
        frame_length_ = DefaultFrameLength;
    }

    roc_log(LogDebug, "sox source: opening: driver=%s path=%s", driver, path);

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

SoxSource::~SoxSource() {
    const status::StatusCode code = close();
    if (code != status::StatusOK) {
        roc_log(LogError, "sox source: close failed: status=%s",
                status::code_to_str(code));
    }
}

status::StatusCode SoxSource::init_status() const {
    return init_status_;
}

DeviceType SoxSource::type() const {
    return DeviceType_Source;
}

ISink* SoxSource::to_sink() {
    return NULL;
}

ISource* SoxSource::to_source() {
    return this;
}

audio::SampleSpec SoxSource::sample_spec() const {
    return frame_spec_;
}

core::nanoseconds_t SoxSource::frame_length() const {
    return frame_length_;
}

bool SoxSource::has_state() const {
    return true;
}

DeviceState SoxSource::state() const {
    if (paused_) {
        return DeviceState_Paused;
    } else {
        return DeviceState_Active;
    }
}

status::StatusCode SoxSource::pause() {
    if (paused_) {
        return status::StatusOK;
    }

    if (!input_) {
        roc_panic("sox source: not opened");
    }

    roc_log(LogDebug, "sox source: pausing: driver=%s path=%s", driver_.c_str(),
            path_.c_str());

    const status::StatusCode close_code = close_();
    if (close_code != status::StatusOK) {
        return close_code;
    }

    paused_ = true;

    return status::StatusOK;
}

status::StatusCode SoxSource::resume() {
    if (!paused_) {
        return status::StatusOK;
    }

    roc_log(LogDebug, "sox source: resuming: driver=%s path=%s", driver_.c_str(),
            path_.c_str());

    if (!input_) {
        const status::StatusCode code = open_();
        if (code != status::StatusOK) {
            return code;
        }
    }

    paused_ = false;

    return status::StatusOK;
}

bool SoxSource::has_latency() const {
    return false;
}

bool SoxSource::has_clock() const {
    return true;
}

status::StatusCode SoxSource::rewind() {
    roc_log(LogDebug, "sox source: rewinding: driver=%s path=%s", driver_.c_str(),
            path_.c_str());

    if (input_) {
        const status::StatusCode close_code = close_();
        if (close_code != status::StatusOK) {
            return close_code;
        }
    }

    const status::StatusCode code = open_();
    if (code != status::StatusOK) {
        return code;
    }

    paused_ = false;

    return status::StatusOK;
}

void SoxSource::reclock(core::nanoseconds_t timestamp) {
    // no-op
}

status::StatusCode SoxSource::read(audio::Frame& frame,
                                   packet::stream_timestamp_t duration,
                                   audio::FrameReadMode mode) {
    if (!input_ && !paused_) {
        roc_panic("sox source: read: non-open input device");
    }

    if (paused_) {
        return status::StatusFinish;
    }

    if (!frame_factory_.reallocate_frame(
            frame, frame_spec_.stream_timestamp_2_bytes(duration))) {
        return status::StatusNoMem;
    }

    frame.set_raw(true);

    audio::sample_t* frame_data = frame.raw_samples();
    size_t frame_left = frame.num_raw_samples();
    size_t frame_size = 0;

    sox_sample_t* buffer_data = buffer_.data();

    SOX_SAMPLE_LOCALS;

    size_t clips = 0;
    (void)clips;

    while (frame_left != 0) {
        size_t n_samples = frame_left;
        if (n_samples > buffer_size_) {
            n_samples = buffer_size_;
        }

        n_samples = sox_read(input_, buffer_data, n_samples);
        if (n_samples == 0) {
            roc_log(LogDebug, "sox source: got eof from sox");
            break;
        }

        for (size_t n = 0; n < n_samples; n++) {
            frame_data[n] = (float)SOX_SAMPLE_TO_FLOAT_32BIT(buffer_data[n], clips);
        }

        frame_data += n_samples;
        frame_left -= n_samples;
        frame_size += n_samples;
    }

    if (frame_size == 0) {
        return status::StatusFinish;
    }

    frame.set_num_raw_samples(frame_size);
    frame.set_duration(frame_size / frame_spec_.num_channels());

    if (frame.duration() < duration) {
        return status::StatusPart;
    }

    return status::StatusOK;
}

status::StatusCode SoxSource::close() {
    return close_();
}

void SoxSource::dispose() {
    arena().dispose_object(*this);
}

status::StatusCode SoxSource::init_names_(const char* driver, const char* path) {
    if (driver) {
        if (!driver_.assign(driver)) {
            roc_log(LogError, "sox source: can't allocate string");
            return status::StatusNoMem;
        }
    }

    if (path) {
        if (!path_.assign(path)) {
            roc_log(LogError, "sox source: can't allocate string");
            return status::StatusNoMem;
        }
    }

    return status::StatusOK;
}

status::StatusCode SoxSource::init_buffer_() {
    buffer_size_ = in_spec_.ns_2_samples_overall(frame_length_);
    if (buffer_size_ == 0) {
        roc_log(LogError, "sox source: buffer size is zero");
        return status::StatusBadConfig;
    }

    if (!buffer_.resize(buffer_size_)) {
        roc_log(LogError, "sox source: can't allocate sample buffer");
        return status::StatusNoMem;
    }

    return status::StatusOK;
}

status::StatusCode SoxSource::open_() {
    memset(&in_signal_, 0, sizeof(in_signal_));
    in_signal_.rate = (sox_rate_t)in_spec_.sample_rate();
    in_signal_.channels = (unsigned)in_spec_.num_channels();
    in_signal_.precision = (unsigned)in_spec_.pcm_bit_width();

    input_ = sox_open_read(path_.is_empty() ? NULL : path_.c_str(), &in_signal_, NULL,
                           driver_.is_empty() ? NULL : driver_.c_str());
    if (!input_) {
        roc_log(LogInfo, "sox source: can't open: driver=%s path=%s", driver_.c_str(),
                path_.c_str());
        return status::StatusErrDevice;
    }

    const unsigned long requested_rate = (unsigned long)in_signal_.rate;
    const unsigned long actual_rate = (unsigned long)input_->signal.rate;

    if (requested_rate != 0 && requested_rate != actual_rate) {
        roc_log(LogError,
                "sox source:"
                " can't open input device with the requested sample rate:"
                " required_by_input=%lu requested_by_user=%lu",
                actual_rate, requested_rate);
        return status::StatusErrDevice;
    }

    const unsigned long requested_chans = (unsigned long)in_signal_.channels;
    const unsigned long actual_chans = (unsigned long)input_->signal.channels;

    if (requested_chans != 0 && requested_chans != actual_chans) {
        roc_log(LogError,
                "sox source:"
                " can't open input device with the requested channel count:"
                " required_by_input=%lu requested_by_user=%lu",
                actual_chans, requested_chans);
        return status::StatusErrDevice;
    }

    const unsigned long requested_bits = (unsigned long)in_signal_.precision;
    const unsigned long actual_bits = (unsigned long)input_->signal.precision;

    if (requested_bits != 0 && requested_bits != actual_bits) {
        roc_log(LogError,
                "sox source:"
                " can't open input device with the requested subformat:"
                " supported=s%lu requested=s%lu",
                actual_bits, requested_bits);
        return status::StatusErrDevice;
    }

    in_spec_.set_sample_rate(actual_rate);
    in_spec_.channel_set().set_layout(audio::ChanLayout_Surround);
    in_spec_.channel_set().set_order(audio::ChanOrder_Smpte);
    in_spec_.channel_set().set_count(actual_chans);

    frame_spec_ = in_spec_;
    frame_spec_.set_format(audio::Format_Pcm);
    frame_spec_.set_pcm_subformat(audio::PcmSubformat_Raw);

    roc_log(LogInfo, "sox source: input output %s",
            audio::sample_spec_to_str(in_spec_).c_str());

    return status::StatusOK;
}

status::StatusCode SoxSource::close_() {
    if (!input_) {
        return status::StatusOK;
    }

    roc_log(LogInfo, "sox source: closing input");

    const int err = sox_close(input_);
    input_ = NULL;

    if (err != SOX_SUCCESS) {
        roc_log(LogError, "sox source: can't close input: %s", sox_strerror(err));
        return status::StatusErrDevice;
    }

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
