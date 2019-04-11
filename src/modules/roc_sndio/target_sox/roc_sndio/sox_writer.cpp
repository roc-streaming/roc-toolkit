/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/sox_writer.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_sndio/sox.h"

namespace roc {
namespace sndio {

SoxWriter::SoxWriter(core::IAllocator& allocator,
                     packet::channel_mask_t channels,
                     size_t sample_rate)
    : output_(NULL)
    , allocator_(allocator)
    , is_file_(false) {
    size_t n_channels = packet::num_channels(channels);
    if (n_channels == 0) {
        roc_panic("sox writer: # of channels is zero");
    }

    memset(&out_signal_, 0, sizeof(out_signal_));
    out_signal_.rate = sample_rate;
    out_signal_.channels = (unsigned)n_channels;
    out_signal_.precision = SOX_SAMPLE_PRECISION;

    buffer_size_ = sox_get_globals()->bufsiz;
}

SoxWriter::~SoxWriter() {
    close_();
}

bool SoxWriter::open(const char* name, const char* type) {
    roc_log(LogDebug, "sox writer: opening: name=%s type=%s", name, type);

    if (buffer_ || output_) {
        roc_panic("sox writer: can't call open() more than once");
    }

    if (!prepare_()) {
        return false;
    }

    if (!open_(name, type)) {
        return false;
    }

    return true;
}

size_t SoxWriter::sample_rate() const {
    if (!output_) {
        roc_panic("sox writer: sample_rate: non-open output file or device");
    }
    return size_t(output_->signal.rate);
}

bool SoxWriter::is_file() const {
    if (!output_) {
        roc_panic("sox writer: is_file: non-open output file or device");
    }
    return is_file_;
}

size_t SoxWriter::frame_size() const {
    return buffer_size_;
}

void SoxWriter::write(audio::Frame& frame) {
    const audio::sample_t* input_data = frame.data();
    size_t input_size = frame.size();

    sox_sample_t* buffer_data = buffer_.get();
    size_t buffer_pos = 0;

    SOX_SAMPLE_LOCALS;

    size_t clips = 0;

    while (input_size > 0) {
        for (; buffer_pos < buffer_size_ && input_size > 0; buffer_pos++) {
            buffer_data[buffer_pos] = SOX_FLOAT_32BIT_TO_SAMPLE(*input_data, clips);
            input_data++;
            input_size--;
        }

        if (buffer_pos == buffer_size_) {
            write_(buffer_data, buffer_pos);
            buffer_pos = 0;
        }
    }

    write_(buffer_data, buffer_pos);
}

bool SoxWriter::prepare_() {
    buffer_.reset(new (allocator_) sox_sample_t[buffer_size_], allocator_);

    if (!buffer_) {
        roc_log(LogError, "sox writer: can't allocate sox buffer");
        return false;
    }

    return true;
}

bool SoxWriter::open_(const char* name, const char* type) {
    if (!sox_defaults(&name, &type)) {
        roc_log(LogError, "sox writer: can't detect defaults: name=%s type=%s", name,
                type);
        return false;
    }

    roc_log(LogInfo, "sox writer: name=%s type=%s", name, type);

    output_ = sox_open_write(name, &out_signal_, NULL, type, NULL, NULL);
    if (!output_) {
        roc_log(LogError, "sox writer: can't open writer: name=%s type=%s", name, type);
        return false;
    }

    is_file_ = !(output_->handler.flags & SOX_FILE_DEVICE);

    unsigned long in_rate = (unsigned long)out_signal_.rate;
    unsigned long out_rate = (unsigned long)output_->signal.rate;

    if (in_rate != 0 && in_rate != out_rate) {
        roc_log(
            LogError,
            "sox writer: can't open output file or device with the required sample rate: "
            "required_by_output=%lu requested_by_user=%lu",
            out_rate, in_rate);
        return false;
    }

    roc_log(LogInfo,
            "sox writer:"
            " bits=%lu out_rate=%lu in_rate=%lu ch=%lu is_file=%d",
            (unsigned long)output_->encoding.bits_per_sample, out_rate, in_rate,
            (unsigned long)output_->signal.channels, (int)is_file_);

    return true;
}

void SoxWriter::write_(const sox_sample_t* data, size_t size) {
    if (size > 0) {
        if (sox_write(output_, data, size) != size) {
            roc_log(LogError, "sox writer: failed to write output buffer");
        }
    }
}

void SoxWriter::close_() {
    if (!output_) {
        return;
    }

    roc_log(LogDebug, "sox writer: closing output");

    int err = sox_close(output_);
    if (err != SOX_SUCCESS) {
        roc_panic("sox writer: can't close output: %s", sox_strerror(err));
    }

    output_ = NULL;
}

} // namespace sndio
} // namespace roc
