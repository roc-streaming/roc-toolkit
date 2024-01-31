/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/frame.h"
#include "roc_core/panic.h"
#include "roc_core/print_buffer.h"
#include "roc_core/printer.h"

namespace roc {
namespace audio {

Frame::Frame(sample_t* samples, size_t num_samples)
    : bytes_((uint8_t*)samples)
    , num_bytes_(num_samples * sizeof(sample_t))
    , flags_(0)
    , duration_(0)
    , capture_timestamp_(0) {
    if (!samples) {
        roc_panic("frame: samples buffer is null");
    }
}

Frame::Frame(uint8_t* bytes, size_t num_bytes)
    : bytes_(bytes)
    , num_bytes_(num_bytes)
    , flags_(0)
    , duration_(0)
    , capture_timestamp_(0) {
    if (!bytes) {
        roc_panic("frame: bytes buffer is null");
    }
}

unsigned Frame::flags() const {
    return flags_;
}

void Frame::set_flags(unsigned flags) {
    flags_ = flags;
}

bool Frame::is_raw() const {
    return (flags_ & FlagNotRaw) == 0;
}

sample_t* Frame::raw_samples() const {
    if (flags_ & FlagNotRaw) {
        roc_panic("frame: frame is not in raw format");
    }

    return (sample_t*)bytes_;
}

size_t Frame::num_raw_samples() const {
    if (flags_ & FlagNotRaw) {
        roc_panic("frame: frame is not in raw format");
    }

    return num_bytes_ / sizeof(sample_t);
}

uint8_t* Frame::bytes() const {
    return bytes_;
}

size_t Frame::num_bytes() const {
    return num_bytes_;
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
        !(flags_ & FlagNotRaw) ? 'r' : '.',
        !(flags_ & FlagNonblank) ? 'b' : '.',
        (flags_ & FlagIncomplete) ? 'i' : '.',
        (flags_ & FlagDrops) ? 'd' : '.',
        '\0',
    };

    core::Printer p;
    p.writef("@ frame flags=[%s] dur=%lu cts=%lld\n", flags_str, (unsigned long)duration_,
             (long long)capture_timestamp_);

    if (flags_ & FlagNotRaw) {
        core::print_buffer(bytes(), num_bytes());
    } else {
        core::print_buffer(raw_samples(), num_raw_samples());
    }
}

} // namespace audio
} // namespace roc
