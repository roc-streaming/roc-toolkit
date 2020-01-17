/*
 * Copyright (c) 2017 Roc authors
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

Frame::Frame(sample_t* data, size_t size)
    : data_(data)
    , size_(size)
    , flags_(0) {
    if (!data) {
        roc_panic("frame: can't create frame for null data");
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

sample_t* Frame::data() const {
    return data_;
}

size_t Frame::size() const {
    return size_;
}

void Frame::print() const {
    core::print_buffer(data_, size_);
}

} // namespace audio
} // namespace roc
