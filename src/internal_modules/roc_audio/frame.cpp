/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/frame.h"
#include "roc_core/panic.h"
#include "roc_core/print_memory.h"
#include "roc_core/printer.h"

namespace roc {
namespace audio {

Frame::Frame(core::IPool& frame_pool)
    : core::RefCounted<Frame, core::PoolAllocation>(frame_pool) {
    clear();
}

void Frame::clear() {
    buffer_.reset();
    is_raw_ = false;
    flags_ = 0;
    duration_ = 0;
    capture_timestamp_ = 0;
}

unsigned Frame::flags() const {
    return flags_;
}

bool Frame::has_flags(unsigned flags) const {
    return (flags_ & flags) == flags;
}

void Frame::set_flags(unsigned flags) {
    flags_ = (uint16_t)flags;
}

const core::Slice<uint8_t>& Frame::buffer() {
    return buffer_;
}

void Frame::set_buffer(const core::Slice<uint8_t>& new_buffer) {
    if (buffer_) {
        roc_panic("frame: buffer already set");
    }
    if (!new_buffer) {
        roc_panic("frame: attempt to set empty buffer");
    }

    buffer_ = new_buffer;
}

bool Frame::is_raw() const {
    return is_raw_;
}

void Frame::set_raw(bool raw) {
    is_raw_ = raw;
}

sample_t* Frame::raw_samples() const {
    if (!is_raw_) {
        roc_panic("frame: frame is not in raw format");
    }

    return (sample_t*)buffer_.data();
}

size_t Frame::num_raw_samples() const {
    if (!is_raw_) {
        roc_panic("frame: frame is not in raw format");
    }

    return buffer_.size() / sizeof(sample_t);
}

void Frame::set_num_raw_samples(size_t n_samples) {
    if (!is_raw_) {
        roc_panic("frame: frame is not in raw format");
    }

    if (!buffer_) {
        roc_panic("frame: frame does not have a buffer");
    }

    if (buffer_.capacity() < n_samples * sizeof(sample_t)) {
        roc_panic("frame: frame buffer does not have enough capacity:"
                  " requested=%lu available=%lu",
                  (unsigned long)n_samples,
                  (unsigned long)buffer_.capacity() / sizeof(sample_t));
    }

    buffer_.reslice(0, n_samples * sizeof(sample_t));
}

uint8_t* Frame::bytes() const {
    return buffer_.data();
}

size_t Frame::num_bytes() const {
    return buffer_.size();
}

void Frame::set_num_bytes(size_t n_bytes) {
    if (!buffer_) {
        roc_panic("frame: frame does not have a buffer");
    }

    if (buffer_.capacity() < n_bytes) {
        roc_panic("frame: frame buffer does not have enough capacity:"
                  " requested=%lu available=%lu",
                  (unsigned long)n_bytes, (unsigned long)buffer_.capacity());
    }

    buffer_.reslice(0, n_bytes);
}

bool Frame::has_duration() const {
    return duration_ != 0;
}

packet::stream_timestamp_t Frame::duration() const {
    if (duration_ == 0) {
        roc_panic("frame: invalid zero duration");
    }

    return duration_;
}

void Frame::set_duration(packet::stream_timestamp_t duration) {
    if (duration == 0) {
        roc_panic("frame: invalid zero duration");
    }

    duration_ = duration;
}

bool Frame::has_capture_timestamp() const {
    return capture_timestamp_ != 0;
}

core::nanoseconds_t Frame::capture_timestamp() const {
    return capture_timestamp_;
}

void Frame::set_capture_timestamp(core::nanoseconds_t capture_ts) {
    if (capture_ts < 0) {
        roc_panic("frame: invalid negative cts: %lld", (long long)capture_ts);
    }

    capture_timestamp_ = capture_ts;
}

void Frame::print() const {
    char flags_str[] = {
        (flags_ & HasSignal) ? 's' : '-',
        (flags_ & HasGaps) ? 'g' : '-',
        (flags_ & HasDrops) ? 'd' : '-',
        '\0',
    };

    core::Printer p;
    p.writef("@ frame flags=[%s] raw=%d dur=%lu cts=%lld\n", flags_str, (int)!!is_raw_,
             (unsigned long)duration_, (long long)capture_timestamp_);

    if (is_raw_) {
        core::print_memory(raw_samples(), num_raw_samples());
    } else {
        core::print_memory(bytes(), num_bytes());
    }
}

} // namespace audio
} // namespace roc
