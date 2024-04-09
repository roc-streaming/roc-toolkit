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

namespace roc {
namespace sndio {

SoxSource::SoxSource(core::IArena& arena, const Config& config, DriverType type)
    : driver_name_(arena)
    , input_name_(arena)
    , buffer_(arena)
    , buffer_size_(0)
    , input_(NULL)
    , is_file_(false)
    , eof_(false)
    , paused_(false)
    , valid_(false) {
    BackendMap::instance();

    if (config.latency != 0) {
        roc_log(LogError, "sox source: setting io latency not supported by sox backend");
        return;
    }

    sample_spec_ = config.sample_spec;

    if (type == DriverType_File) {
        if (!sample_spec_.is_empty()) {
            roc_log(LogError, "sox source: setting io encoding for files not supported");
            return;
        }
    } else {
        sample_spec_.use_defaults(audio::Sample_RawFormat, audio::ChanLayout_Surround,
                                  audio::ChanOrder_Smpte, audio::ChanMask_Surround_Stereo,
                                  0);

        if (!sample_spec_.is_raw()) {
            roc_log(LogError, "sox sink: sample format can be only \"-\" or \"%s\"",
                    audio::pcm_format_to_str(audio::Sample_RawFormat));
            return;
        }
    }

    frame_length_ = config.frame_length;

    if (frame_length_ == 0) {
        roc_log(LogError, "sox source: frame length is zero");
        return;
    }

    memset(&in_signal_, 0, sizeof(in_signal_));
    in_signal_.rate = (sox_rate_t)sample_spec_.sample_rate();
    in_signal_.channels = (unsigned)sample_spec_.num_channels();
    in_signal_.precision = SOX_SAMPLE_PRECISION;

    valid_ = true;
}

SoxSource::~SoxSource() {
    close_();
}

bool SoxSource::is_valid() const {
    return valid_;
}

bool SoxSource::open(const char* driver, const char* path) {
    roc_panic_if(!valid_);

    roc_log(LogInfo, "sox source: opening: driver=%s path=%s", driver, path);

    if (buffer_.size() != 0 || input_) {
        roc_panic("sox source: can't call open() more than once");
    }

    if (!setup_names_(driver, path)) {
        return false;
    }

    if (!open_()) {
        return false;
    }

    if (!setup_buffer_()) {
        return false;
    }

    return true;
}

ISink* SoxSource::to_sink() {
    return NULL;
}

ISource* SoxSource::to_source() {
    return this;
}

DeviceType SoxSource::type() const {
    return DeviceType_Source;
}

DeviceState SoxSource::state() const {
    roc_panic_if(!valid_);

    if (paused_) {
        return DeviceState_Paused;
    } else {
        return DeviceState_Active;
    }
}

void SoxSource::pause() {
    roc_panic_if(!valid_);

    if (paused_) {
        return;
    }

    if (!input_) {
        roc_panic("sox source: pause: non-open device");
    }

    roc_log(LogDebug, "sox source: pausing: driver=%s input=%s", driver_name_.c_str(),
            input_name_.c_str());

    if (!is_file_) {
        close_();
    }

    paused_ = true;
}

bool SoxSource::resume() {
    roc_panic_if(!valid_);

    if (!paused_) {
        return true;
    }

    roc_log(LogDebug, "sox source: resuming: driver=%s input=%s", driver_name_.c_str(),
            input_name_.c_str());

    if (!input_) {
        if (!open_()) {
            roc_log(LogError, "sox source: open failed when resuming: driver=%s input=%s",
                    driver_name_.c_str(), input_name_.c_str());
            return false;
        }
    }

    paused_ = false;
    return true;
}

bool SoxSource::restart() {
    roc_panic_if(!valid_);

    roc_log(LogDebug, "sox source: restarting: driver=%s input=%s", driver_name_.c_str(),
            input_name_.c_str());

    if (is_file_ && !eof_) {
        if (!seek_(0)) {
            roc_log(LogError,
                    "sox source: seek failed when restarting: driver=%s input=%s",
                    driver_name_.c_str(), input_name_.c_str());
            return false;
        }
    } else {
        if (is_file_) {
            sample_spec_.clear();
        }

        if (input_) {
            close_();
        }

        if (!open_()) {
            roc_log(LogError,
                    "sox source: open failed when restarting: driver=%s input=%s",
                    driver_name_.c_str(), input_name_.c_str());
            return false;
        }
    }

    paused_ = false;
    eof_ = false;

    return true;
}

audio::SampleSpec SoxSource::sample_spec() const {
    roc_panic_if(!valid_);

    if (!input_) {
        roc_panic("sox source: sample_rate(): non-open output file or device");
    }

    return sample_spec_;
}

core::nanoseconds_t SoxSource::latency() const {
    roc_panic_if(!valid_);

    if (!input_) {
        roc_panic("sox source: latency(): non-open output file or device");
    }

    return 0;
}

bool SoxSource::has_latency() const {
    roc_panic_if(!valid_);

    if (!input_) {
        roc_panic("sox source: has_latency(): non-open input file or device");
    }

    return false;
}

bool SoxSource::has_clock() const {
    roc_panic_if(!valid_);

    if (!input_) {
        roc_panic("sox source: has_clock(): non-open input file or device");
    }

    return !is_file_;
}

void SoxSource::reclock(core::nanoseconds_t) {
    // no-op
}

bool SoxSource::read(audio::Frame& frame) {
    roc_panic_if(!valid_);

    if (paused_ || eof_) {
        return false;
    }

    if (!input_) {
        roc_panic("sox source: read: non-open input file or device");
    }

    audio::sample_t* frame_data = frame.raw_samples();
    size_t frame_left = frame.num_raw_samples();

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
    }

    if (frame_left == frame.num_raw_samples()) {
        return false;
    }

    if (frame_left != 0) {
        memset(frame_data, 0, frame_left * sizeof(audio::sample_t));
    }

    return true;
}

bool SoxSource::seek_(uint64_t offset) {
    roc_panic_if(!valid_);

    if (!input_) {
        roc_panic("sox source: seek: non-open input file or device");
    }

    if (!is_file_) {
        roc_panic("sox source: seek: not a file");
    }

    roc_log(LogDebug, "sox source: resetting position to %lu", (unsigned long)offset);

    int err = sox_seek(input_, offset, SOX_SEEK_SET);
    if (err != SOX_SUCCESS) {
        roc_log(LogError, "sox source: can't reset position to %lu: %s",
                (unsigned long)offset, sox_strerror(err));
        return false;
    }

    return true;
}

bool SoxSource::setup_names_(const char* driver, const char* path) {
    if (driver) {
        if (!driver_name_.assign(driver)) {
            roc_log(LogError, "sox source: can't allocate string");
            return false;
        }
    }

    if (path) {
        if (!input_name_.assign(path)) {
            roc_log(LogError, "sox source: can't allocate string");
            return false;
        }
    }

    return true;
}

bool SoxSource::setup_buffer_() {
    buffer_size_ = sample_spec_.ns_2_samples_overall(frame_length_);
    if (buffer_size_ == 0) {
        roc_log(LogError, "sox source: buffer size is zero");
        return false;
    }
    if (!buffer_.resize(buffer_size_)) {
        roc_log(LogError, "sox source: can't allocate sample buffer");
        return false;
    }

    return true;
}

bool SoxSource::open_() {
    if (input_) {
        roc_panic("sox source: already opened");
    }

    input_ =
        sox_open_read(input_name_.is_empty() ? NULL : input_name_.c_str(), &in_signal_,
                      NULL, driver_name_.is_empty() ? NULL : driver_name_.c_str());
    if (!input_) {
        roc_log(LogInfo, "sox source: can't open: driver=%s input=%s",
                driver_name_.c_str(), input_name_.c_str());
        return false;
    }

    is_file_ = !(input_->handler.flags & SOX_FILE_DEVICE);

    const unsigned long requested_rate = (unsigned long)in_signal_.rate;
    const unsigned long actual_rate = (unsigned long)input_->signal.rate;

    if (requested_rate != 0 && requested_rate != actual_rate) {
        roc_log(LogError,
                "sox source:"
                " can't open input file or device with the requested sample rate:"
                " required_by_input=%lu requested_by_user=%lu",
                actual_rate, requested_rate);
        return false;
    }

    const unsigned long requested_chans = (unsigned long)in_signal_.channels;
    const unsigned long actual_chans = (unsigned long)input_->signal.channels;

    if (requested_chans != 0 && requested_chans != actual_chans) {
        roc_log(LogError,
                "sox source:"
                " can't open input file or device with the requested channel count:"
                " required_by_input=%lu requested_by_user=%lu",
                actual_chans, requested_chans);
        return false;
    }

    sample_spec_.set_sample_format(audio::SampleFormat_Pcm);
    sample_spec_.set_pcm_format(audio::Sample_RawFormat);
    sample_spec_.set_sample_rate(actual_rate);
    sample_spec_.channel_set().set_layout(audio::ChanLayout_Surround);
    sample_spec_.channel_set().set_order(audio::ChanOrder_Smpte);
    sample_spec_.channel_set().set_count(actual_chans);

    roc_log(LogInfo,
            "sox source:"
            " opened: bits=%lu rate=%lu req_rate=%lu chans=%lu req_chans=%lu is_file=%d",
            (unsigned long)input_->encoding.bits_per_sample, actual_rate, requested_rate,
            actual_chans, requested_chans, (int)is_file_);

    return true;
}

void SoxSource::close_() {
    if (!input_) {
        return;
    }

    roc_log(LogInfo, "sox source: closing input");

    int err = sox_close(input_);
    if (err != SOX_SUCCESS) {
        roc_panic("sox source: can't close input: %s", sox_strerror(err));
    }

    input_ = NULL;
}

} // namespace sndio
} // namespace roc
