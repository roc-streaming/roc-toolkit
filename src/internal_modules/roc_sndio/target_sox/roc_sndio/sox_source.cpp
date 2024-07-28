/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sox_source.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/backend_map.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

SoxSource::SoxSource(audio::FrameFactory& frame_factory,
                     core::IArena& arena,
                     const Config& config,
                     DriverType driver_type)
    : frame_factory_(frame_factory)
    , driver_type_(driver_type)
    , driver_name_(arena)
    , input_name_(arena)
    , buffer_(arena)
    , buffer_size_(0)
    , input_(NULL)
    , eof_(false)
    , paused_(false)
    , init_status_(status::NoStatus) {
    BackendMap::instance();

    if (config.latency != 0) {
        roc_log(LogError, "sox source: setting io latency not supported by sox backend");
        init_status_ = status::StatusBadConfig;
        return;
    }

    sample_spec_ = config.sample_spec;

    if (driver_type_ == DriverType_File) {
        if (!sample_spec_.is_empty()) {
            roc_log(LogError, "sox source: setting io encoding for files not supported");
            init_status_ = status::StatusBadConfig;
            return;
        }
    } else {
        sample_spec_.use_defaults(audio::Sample_RawFormat, audio::ChanLayout_Surround,
                                  audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo,
                                  0);

        if (!sample_spec_.is_raw()) {
            roc_log(LogError, "sox sink: sample format can be only \"-\" or \"%s\"",
                    audio::pcm_format_to_str(audio::Sample_RawFormat));
            init_status_ = status::StatusBadConfig;
            return;
        }
    }

    frame_length_ = config.frame_length;

    if (frame_length_ == 0) {
        roc_log(LogError, "sox source: frame length is zero");
        init_status_ = status::StatusBadConfig;
        return;
    }

    memset(&in_signal_, 0, sizeof(in_signal_));
    in_signal_.rate = (sox_rate_t)sample_spec_.sample_rate();
    in_signal_.channels = (unsigned)sample_spec_.num_channels();
    in_signal_.precision = SOX_SAMPLE_PRECISION;

    init_status_ = status::StatusOK;
}

SoxSource::~SoxSource() {
    if (input_) {
        roc_panic("sox source: input file is not closed");
    }
}

status::StatusCode SoxSource::init_status() const {
    return init_status_;
}

status::StatusCode SoxSource::open(const char* driver, const char* path) {
    roc_log(LogInfo, "sox source: opening: driver=%s path=%s", driver, path);

    if (buffer_.size() != 0 || input_) {
        roc_panic("sox source: can't call open() more than once");
    }

    status::StatusCode code = status::NoStatus;

    if ((code = init_names_(driver, path)) != status::StatusOK) {
        return code;
    }

    if ((code = open_()) != status::StatusOK) {
        return code;
    }

    if ((code = init_buffer_()) != status::StatusOK) {
        return code;
    }

    return status::StatusOK;
}

status::StatusCode SoxSource::close() {
    return close_();
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
    if (!input_ && !paused_) {
        roc_panic("sox source: not opened");
    }

    return sample_spec_;
}

bool SoxSource::has_state() const {
    return driver_type_ == DriverType_Device;
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

    roc_log(LogDebug, "sox source: pausing: driver=%s input=%s", driver_name_.c_str(),
            input_name_.c_str());

    if (driver_type_ == DriverType_Device) {
        const status::StatusCode close_code = close_();
        if (close_code != status::StatusOK) {
            return close_code;
        }
    }

    paused_ = true;

    return status::StatusOK;
}

status::StatusCode SoxSource::resume() {
    if (!paused_) {
        return status::StatusOK;
    }

    roc_log(LogDebug, "sox source: resuming: driver=%s input=%s", driver_name_.c_str(),
            input_name_.c_str());

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
    return driver_type_ == DriverType_Device;
}

status::StatusCode SoxSource::rewind() {
    roc_log(LogDebug, "sox source: rewinding: driver=%s input=%s", driver_name_.c_str(),
            input_name_.c_str());

    if (driver_type_ == DriverType_File && !eof_) {
        const status::StatusCode code = seek_(0);
        if (code != status::StatusOK) {
            return code;
        }
    } else {
        sample_spec_.clear();

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
    }

    paused_ = false;
    eof_ = false;

    return status::StatusOK;
}

void SoxSource::reclock(core::nanoseconds_t timestamp) {
    // no-op
}

status::StatusCode SoxSource::read(audio::Frame& frame,
                                   packet::stream_timestamp_t duration,
                                   audio::FrameReadMode mode) {
    if (!input_ && !paused_) {
        roc_panic("sox source: read: non-open input file or device");
    }

    if (paused_ || eof_) {
        return status::StatusFinish;
    }

    if (!frame_factory_.reallocate_frame(
            frame, sample_spec_.stream_timestamp_2_bytes(duration))) {
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
            eof_ = true;
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
    frame.set_duration(frame_size / sample_spec_.num_channels());

    if (frame.duration() < duration) {
        return status::StatusPart;
    }

    return status::StatusOK;
}

status::StatusCode SoxSource::init_names_(const char* driver, const char* path) {
    if (driver) {
        if (!driver_name_.assign(driver)) {
            roc_log(LogError, "sox source: can't allocate string");
            return status::StatusNoMem;
        }
    }

    if (path) {
        if (!input_name_.assign(path)) {
            roc_log(LogError, "sox source: can't allocate string");
            return status::StatusNoMem;
        }
    }

    return status::StatusOK;
}

status::StatusCode SoxSource::init_buffer_() {
    buffer_size_ = sample_spec_.ns_2_samples_overall(frame_length_);
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
    if (input_) {
        roc_panic("sox source: already opened");
    }

    input_ =
        sox_open_read(input_name_.is_empty() ? NULL : input_name_.c_str(), &in_signal_,
                      NULL, driver_name_.is_empty() ? NULL : driver_name_.c_str());
    if (!input_) {
        roc_log(LogInfo, "sox source: can't open: driver=%s input=%s",
                driver_name_.c_str(), input_name_.c_str());
        return driver_type_ == DriverType_Device ? status::StatusErrDevice
                                                 : status::StatusErrFile;
    }

    const unsigned long requested_rate = (unsigned long)in_signal_.rate;
    const unsigned long actual_rate = (unsigned long)input_->signal.rate;

    if (requested_rate != 0 && requested_rate != actual_rate) {
        roc_log(LogError,
                "sox source:"
                " can't open input file or device with the requested sample rate:"
                " required_by_input=%lu requested_by_user=%lu",
                actual_rate, requested_rate);
        return driver_type_ == DriverType_Device ? status::StatusErrDevice
                                                 : status::StatusErrFile;
    }

    const unsigned long requested_chans = (unsigned long)in_signal_.channels;
    const unsigned long actual_chans = (unsigned long)input_->signal.channels;

    if (requested_chans != 0 && requested_chans != actual_chans) {
        roc_log(LogError,
                "sox source:"
                " can't open input file or device with the requested channel count:"
                " required_by_input=%lu requested_by_user=%lu",
                actual_chans, requested_chans);
        return driver_type_ == DriverType_Device ? status::StatusErrDevice
                                                 : status::StatusErrFile;
    }

    sample_spec_.set_sample_format(audio::SampleFormat_Pcm);
    sample_spec_.set_pcm_format(audio::Sample_RawFormat);
    sample_spec_.set_sample_rate(actual_rate);
    sample_spec_.channel_set().set_layout(audio::ChanLayout_Surround);
    sample_spec_.channel_set().set_order(audio::ChanOrder_Smpte);
    sample_spec_.channel_set().set_count(actual_chans);

    roc_log(LogInfo,
            "sox source: opened input:"
            " bits=%lu rate=%lu req_rate=%lu chans=%lu req_chans=%lu is_file=%d",
            (unsigned long)input_->encoding.bits_per_sample, actual_rate, requested_rate,
            actual_chans, requested_chans, (int)(driver_type_ == DriverType_File));

    return status::StatusOK;
}

status::StatusCode SoxSource::seek_(uint64_t offset) {
    roc_log(LogDebug, "sox source: resetting position to %lu", (unsigned long)offset);

    const int err = sox_seek(input_, offset, SOX_SEEK_SET);
    if (err != SOX_SUCCESS) {
        roc_log(LogError, "sox source: can't reset position to %lu: %s",
                (unsigned long)offset, sox_strerror(err));
        return status::StatusErrFile;
    }

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
        return driver_type_ == DriverType_File ? status::StatusErrFile
                                               : status::StatusErrDevice;
    }

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
