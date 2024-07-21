/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sox_sink.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/backend_map.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

SoxSink::SoxSink(audio::FrameFactory& frame_factory,
                 core::IArena& arena,
                 const Config& config,
                 DriverType driver_type)
    : driver_type_(driver_type)
    , driver_name_(arena)
    , output_name_(arena)
    , output_(NULL)
    , buffer_(arena)
    , buffer_size_(0)
    , paused_(false)
    , init_status_(status::NoStatus) {
    BackendMap::instance();

    if (config.latency != 0) {
        roc_log(LogError, "sox sink: setting io latency not supported by sox backend");
        init_status_ = status::StatusBadConfig;
        return;
    }

    sample_spec_ = config.sample_spec;

    if (driver_type_ == DriverType_File) {
        sample_spec_.use_defaults(audio::Sample_RawFormat, audio::ChanLayout_Surround,
                                  audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo,
                                  44100);
    } else {
        sample_spec_.use_defaults(audio::Sample_RawFormat, audio::ChanLayout_Surround,
                                  audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo,
                                  0);
    }

    if (!sample_spec_.is_raw()) {
        roc_log(LogError, "sox sink: sample format can be only \"-\" or \"%s\"",
                audio::pcm_format_to_str(audio::Sample_RawFormat));
        init_status_ = status::StatusBadConfig;
        return;
    }

    frame_length_ = config.frame_length;

    if (frame_length_ == 0) {
        roc_log(LogError, "sox sink: frame length is zero");
        init_status_ = status::StatusBadConfig;
        return;
    }

    memset(&out_signal_, 0, sizeof(out_signal_));
    out_signal_.rate = (sox_rate_t)sample_spec_.sample_rate();
    out_signal_.channels = (unsigned)sample_spec_.num_channels();
    out_signal_.precision = SOX_SAMPLE_PRECISION;

    init_status_ = status::StatusOK;
}

SoxSink::~SoxSink() {
    if (output_) {
        roc_panic("sox sink: output file is not closed");
    }
}

status::StatusCode SoxSink::init_status() const {
    return init_status_;
}

status::StatusCode SoxSink::open(const char* driver, const char* path) {
    roc_log(LogDebug, "sox sink: opening: driver=%s path=%s", driver, path);

    if (buffer_.size() != 0 || output_) {
        roc_panic("sox sink: can't call open() more than once");
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

status::StatusCode SoxSink::close() {
    return close_();
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
    if (!output_) {
        roc_panic("sox sink: not opened");
    }

    return sample_spec_;
}

bool SoxSink::has_state() const {
    return driver_type_ == DriverType_Device;
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

    roc_log(LogDebug, "sox sink: pausing: driver=%s output=%s", driver_name_.c_str(),
            output_name_.c_str());

    if (driver_type_ == DriverType_Device) {
        const status::StatusCode close_code = close_();
        if (close_code != status::StatusOK) {
            return close_code;
        }
    }

    paused_ = true;

    return status::StatusOK;
}

status::StatusCode SoxSink::resume() {
    if (!paused_) {
        return status::StatusOK;
    }

    roc_log(LogDebug, "sox sink: resuming: driver=%s output=%s", driver_name_.c_str(),
            output_name_.c_str());

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
    return driver_type_ == DriverType_Device;
}

status::StatusCode SoxSink::write(audio::Frame& frame) {
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
            const status::StatusCode code = write_(buffer_data, buffer_pos);
            if (code != status::StatusOK) {
                return code;
            }
            buffer_pos = 0;
        }
    }

    if (buffer_pos > 0) {
        const status::StatusCode code = write_(buffer_data, buffer_pos);
        if (code != status::StatusOK) {
            return code;
        }
    }

    return status::StatusOK;
}

status::StatusCode SoxSink::flush() {
    if (output_ != NULL && driver_type_ == DriverType_File && output_->fp != NULL) {
        if (fflush((FILE*)output_->fp) != 0) {
            return status::StatusErrFile;
        }
    }

    return status::StatusOK;
}

status::StatusCode SoxSink::init_names_(const char* driver, const char* path) {
    if (driver) {
        if (!driver_name_.assign(driver)) {
            roc_log(LogError, "sox sink: can't allocate string");
            return status::StatusNoMem;
        }
    }

    if (path) {
        if (!output_name_.assign(path)) {
            roc_log(LogError, "sox sink: can't allocate string");
            return status::StatusNoMem;
        }
    }

    return status::StatusOK;
}

status::StatusCode SoxSink::init_buffer_() {
    buffer_size_ = sample_spec_.ns_2_samples_overall(frame_length_);
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
    output_ = sox_open_write(
        output_name_.is_empty() ? NULL : output_name_.c_str(), &out_signal_, NULL,
        driver_name_.is_empty() ? NULL : driver_name_.c_str(), NULL, NULL);
    if (!output_) {
        roc_log(LogDebug, "sox sink: can't open: driver=%s path=%s", driver_name_.c_str(),
                output_name_.c_str());
        return driver_type_ == DriverType_File ? status::StatusErrFile
                                               : status::StatusErrDevice;
    }

    const unsigned long requested_rate = (unsigned long)out_signal_.rate;
    const unsigned long actual_rate = (unsigned long)output_->signal.rate;

    if (requested_rate != 0 && requested_rate != actual_rate) {
        roc_log(LogError,
                "sox sink:"
                " can't open output file or device with the requested sample rate:"
                " required_by_output=%lu requested_by_user=%lu",
                actual_rate, requested_rate);
        return driver_type_ == DriverType_File ? status::StatusErrFile
                                               : status::StatusErrDevice;
    }

    const unsigned long requested_chans = (unsigned long)out_signal_.channels;
    const unsigned long actual_chans = (unsigned long)output_->signal.channels;

    if (requested_chans != 0 && requested_chans != actual_chans) {
        roc_log(LogError,
                "sox sink:"
                " can't open output file or device with the requested channel count:"
                " required_by_output=%lu requested_by_user=%lu",
                actual_chans, requested_chans);
        return driver_type_ == DriverType_File ? status::StatusErrFile
                                               : status::StatusErrDevice;
    }

    sample_spec_.set_sample_rate(actual_rate);
    sample_spec_.channel_set().set_layout(audio::ChanLayout_Surround);
    sample_spec_.channel_set().set_order(audio::ChanOrder_Smpte);
    sample_spec_.channel_set().set_count(actual_chans);

    roc_log(LogInfo,
            "sox sink: opened output:"
            " bits=%lu rate=%lu req_rate=%lu chans=%lu req_chans=%lu is_file=%d",
            (unsigned long)output_->encoding.bits_per_sample, actual_rate, requested_rate,
            actual_chans, requested_chans, (int)(driver_type_ == DriverType_File));

    return status::StatusOK;
}

status::StatusCode SoxSink::write_(const sox_sample_t* samples, size_t n_samples) {
    if (n_samples > 0) {
        if (sox_write(output_, samples, n_samples) != n_samples) {
            roc_log(LogError, "sox sink: failed to write output buffer");
            return driver_type_ == DriverType_File ? status::StatusErrFile
                                                   : status::StatusErrDevice;
        }
    }

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
        return driver_type_ == DriverType_File ? status::StatusErrFile
                                               : status::StatusErrDevice;
    }

    return status::StatusOK;
}

} // namespace sndio
} // namespace roc
