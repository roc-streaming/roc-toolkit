/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/frame.h"

namespace roc {
namespace audio {

Frame::Frame()
    : flags_(0) {
}

Frame::Frame(const core::Slice<sample_t>& samples)
    : samples_(samples)
    , flags_(0) {
}

void Frame::add_flags(unsigned fl) {
    flags_ |= fl;
}

bool Frame::is_empty() const {
    return flags_ & FlagEmpty;
}

bool Frame::has_skip() const {
    return flags_ & FlagSkip;
}

const core::Slice<sample_t>& Frame::samples() const {
    return samples_;
}

void Frame::set_samples(const core::Slice<sample_t>& samples) {
    samples_ = samples;
}

} // namespace audio
} // namespace roc
