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

namespace roc {
namespace audio {

Frame::Frame(sample_t* samples, size_t num_samples, core::nanoseconds_t ts)
    : samples_(samples)
    , num_samples_(num_samples)
    , flags_(0)
    , capture_timestamp_(ts) {
    if (!samples) {
        roc_panic("frame: can't create frame with null samples");
    }
}

void Frame::set_flags(unsigned fl) {
    if (flags_) {
        roc_panic("frame: can't set flags more than once");
    }
    flags_ = fl;
}

unsigned Frame::flags() const {
    return flags_;
}

sample_t* Frame::samples() const {
    return samples_;
}

size_t Frame::num_samples() const {
    return num_samples_;
}

void Frame::print() const {
    core::print_buffer(samples_, num_samples_);
}

core::nanoseconds_t Frame::capture_timestamp() const {
    return capture_timestamp_;
}

core::nanoseconds_t& Frame::capture_timestamp() {
    return capture_timestamp_;
}

} // namespace audio
} // namespace roc
