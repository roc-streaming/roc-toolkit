/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sox_source.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_error/error_code.h"
#include "roc_sndio/sox_backend.h"

namespace roc {
namespace sndio {

SoxSource::SoxSource(core::IAllocator& allocator, const Config& config)
    : driver_name_(allocator)
    , input_name_(allocator)
    , buffer_(allocator)
    , buffer_size_(0)
    , input_(NULL)
    , is_file_(false)
    , eof_(false)
    , paused_(false)
    , valid_(false) {
    SoxBackend::instance();

    n_channels_ = packet::num_channels(config.channels);
    if (n_channels_ == 0) {
        roc_log(LogError, "sox source: # of channels is zero");
        return;
    }

    if (config.latency != 0) {
        roc_log(LogError, "sox source: setting io latency not supported by sox backend");
        return;
    }

    frame_length_ = config.frame_length;
    channels_ = config.channels;

    if (frame_length_ == 0) {
        roc_log(LogError, "sox source: frame length is zero");
        return;
    }

    memset(&in_signal_, 0, sizeof(in_signal_));
    in_signal_.rate = config.sample_rate;
    in_signal_.precision = SOX_SAMPLE_PRECISION;

    valid_ = true;
}

SoxSource::~SoxSource() {
    close_();
}

bool SoxSource::valid() const {
    return valid_;
}

bool SoxSource::open(const char* driver, const char* input) {
    roc_panic_if(!valid_);

    roc_log(LogInfo, "sox source: opening: driver=%s input=%s", driver, input);

    if (buffer_.size() != 0 || input_) {
        roc_panic("sox source: can't call open() more than once");
    }

    if (!setup_names_(driver, input)) {
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

size_t SoxSource::sample_rate() const {
    roc_panic_if(!valid_);

    if (!input_) {
        roc_panic("sox source: sample_rate: non-open input file or device");
    }

    return size_t(input_->signal.rate);
}

size_t SoxSource::num_channels() const {
    roc_panic_if(!valid_);

    if (!input_) {
        roc_panic("sox source: sample_rate: non-open input file or device");
    }

    return n_channels_;
}

bool SoxSource::has_clock() const {
    roc_panic_if(!valid_);

    if (!input_) {
        roc_panic("sox source: has_clock: non-open input file or device");
    }

    return !is_file_;
}

ISource::State SoxSource::state() const {
    roc_panic_if(!valid_);

    if (paused_) {
        return Paused;
    } else {
        return Playing;
    }
}

void SoxSource::pause() {
    roc_panic_if(!valid_);

    if (paused_) {
        return;
    }

    if (!input_) {
        roc_panic("sox source: pause: non-open input file or device");
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

ssize_t SoxSource::read(audio::Frame& frame) {
    roc_panic_if(!valid_);

    if (paused_ || eof_) {
        return roc::error::ErrUnknown;
    }

    if (!input_) {
        roc_panic("sox source: read: non-open input file or device");
    }

    audio::sample_t* frame_data = frame.data();
    size_t frame_left = frame.size();

    sox_sample_t* buffer_data = buffer_.data();

    SOX_SAMPLE_LOCALS;

    size_t clips = 0;

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

    if (frame_left == frame.size()) {
        return roc::error::ErrUnknown;
    }

    if (frame_left != 0) {
        memset(frame_data, 0, frame_left * sizeof(audio::sample_t));
    }

    return frame.size() - frame_left;
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

bool SoxSource::setup_names_(const char* driver, const char* input) {
    if (driver) {
        if (!driver_name_.set_str(driver)) {
            roc_log(LogError, "sox source: can't allocate string");
            return false;
        }
    }

    if (input) {
        if (!input_name_.set_str(input)) {
            roc_log(LogError, "sox source: can't allocate string");
            return false;
        }
    }

    return true;
}

bool SoxSource::setup_buffer_() {
    buffer_size_ = packet::ns_to_size(frame_length_, sample_rate(), channels_);
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
        roc_log(LogError, "sox source: can't open: driver=%s input=%s",
                driver_name_.c_str(), input_name_.c_str());
        return false;
    }

    is_file_ = !(input_->handler.flags & SOX_FILE_DEVICE);

    roc_log(LogInfo,
            "sox source:"
            " in_bits=%lu out_bits=%lu in_rate=%lu out_rate=%lu"
            " in_ch=%lu out_ch=%lu is_file=%d",
            (unsigned long)input_->encoding.bits_per_sample,
            (unsigned long)in_signal_.precision, (unsigned long)input_->signal.rate,
            (unsigned long)in_signal_.rate, (unsigned long)input_->signal.channels,
            (unsigned long)in_signal_.channels, (int)is_file_);

    if (input_->signal.channels != n_channels_) {
        roc_log(LogError,
                "sox source: can't open: unsupported # of channels: "
                "expected=%lu actual=%lu",
                (unsigned long)n_channels_, (unsigned long)input_->signal.channels);
        return false;
    }

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
