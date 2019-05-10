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
#include "roc_sndio/sox_controller.h"

namespace roc {
namespace sndio {

SoxSource::SoxSource(core::IAllocator& allocator,
                     packet::channel_mask_t channels,
                     size_t sample_rate,
                     size_t frame_size)
    : input_(NULL)
    , allocator_(allocator)
    , is_file_(false)
    , eof_(false) {
    n_channels_ = packet::num_channels(channels);
    if (n_channels_ == 0) {
        roc_panic("sox source: # of channels is zero");
    }

    if (frame_size == 0) {
        frame_size = SoxController::instance().get_globals().bufsiz / n_channels_;
    }

    memset(&in_signal_, 0, sizeof(in_signal_));
    in_signal_.rate = sample_rate;
    in_signal_.precision = SOX_SAMPLE_PRECISION;

    buffer_size_ = frame_size * n_channels_;
}

SoxSource::~SoxSource() {
    close_();
}

bool SoxSource::open(const char* driver, const char* input) {
    roc_log(LogDebug, "sox source: opening: driver=%s input=%s", driver, input);

    if (buffer_ || input_) {
        roc_panic("sox source: can't call open() more than once");
    }

    if (!prepare_()) {
        return false;
    }

    if (!open_(driver, input)) {
        return false;
    }

    return true;
}

size_t SoxSource::sample_rate() const {
    if (!input_) {
        roc_panic("sox source: sample_rate: non-open input file or device");
    }
    return size_t(input_->signal.rate);
}

bool SoxSource::is_file() const {
    if (!input_) {
        roc_panic("sox source: is_file: non-open input file or device");
    }
    return is_file_;
}

size_t SoxSource::frame_size() const {
    return buffer_size_;
}

ISource::State SoxSource::state() const {
    return Active;
}

void SoxSource::wait_active() const {
    // always active
}

bool SoxSource::read(audio::Frame& frame) {
    if (eof_) {
        return false;
    }

    audio::sample_t* frame_data = frame.data();
    size_t frame_left = frame.size();

    sox_sample_t* buffer_data = buffer_.get();

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
        return false;
    }

    if (frame_left != 0) {
        memset(frame_data, 0, frame_left * sizeof(audio::sample_t));
    }

    return true;
}

bool SoxSource::prepare_() {
    buffer_.reset(new (allocator_) sox_sample_t[buffer_size_], allocator_);

    if (!buffer_) {
        roc_log(LogError, "sox source: can't allocate sox buffer");
        return false;
    }

    return true;
}

bool SoxSource::open_(const char* driver, const char* input) {
    if (!SoxController::instance().fill_defaults(driver, input)) {
        return false;
    }

    roc_log(LogInfo, "sox source: driver=%s input=%s", driver, input);

    if (!(input_ = sox_open_read(input, &in_signal_, NULL, driver))) {
        roc_log(LogError, "sox source: can't open: driver=%s input=%s", driver, input);
        return false;
    }

    is_file_ = !(input_->handler.flags & SOX_FILE_DEVICE);

    roc_log(LogInfo,
            "sox source:"
            " in_bits=%lu out_bits=%lu in_rate=%lu out_rate=%lu"
            " in_ch=%lu, out_ch=%lu, is_file=%d",
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

    roc_log(LogDebug, "sox source: closing input");

    int err = sox_close(input_);
    if (err != SOX_SUCCESS) {
        roc_panic("sox source: can't close input: %s", sox_strerror(err));
    }

    input_ = NULL;
}

} // namespace sndio
} // namespace roc
