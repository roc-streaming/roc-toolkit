/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/mixer.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

sample_t clamp(const sample_t x) {
    if (x > SampleMax) {
        return SampleMax;
    } else if (x < SampleMin) {
        return SampleMin;
    } else {
        return x;
    }
}

} // namespace

Mixer::Mixer(core::BufferPool<sample_t>& pool,
             core::nanoseconds_t frame_length,
             size_t sample_rate,
             packet::channel_mask_t channel_mask)
    : valid_(false) {
    size_t frame_size = packet::ns_to_size(frame_length, sample_rate, channel_mask);
    roc_log(LogDebug, "mixer: initializing: frame_size=%lu", (unsigned long)frame_size);

    if (frame_size == 0) {
        roc_log(LogError, "mixer: frame size cannot be 0");
        return;
    }

    temp_buf_ = new (pool) core::Buffer<sample_t>(pool);
    if (!temp_buf_) {
        roc_log(LogError, "mixer: can't allocate temporary buffer");
        return;
    }

    if (temp_buf_.capacity() < frame_size) {
        roc_log(LogError, "mixer: allocated buffer is too small");
        return;
    }
    temp_buf_.reslice(0, frame_size);

    valid_ = true;
}

bool Mixer::valid() const {
    return valid_;
}

void Mixer::add_input(IReader& reader) {
    roc_panic_if(!valid_);

    readers_.push_back(reader);
}

void Mixer::remove_input(IReader& reader) {
    roc_panic_if(!valid_);

    readers_.remove(reader);
}

ssize_t Mixer::read(Frame& frame) {
    roc_panic_if(!valid_);

    if (readers_.size() == 1) {
        return readers_.front()->read(frame);
    }

    const size_t max_read = temp_buf_.size();

    sample_t* samples = frame.data();
    size_t n_samples = frame.size();

    while (n_samples != 0) {
        size_t n_read = n_samples;
        if (n_read > max_read) {
            n_read = max_read;
        }

        read_(samples, n_read);

        samples += n_read;
        n_samples -= n_read;
    }

    return frame.size();
}

void Mixer::read_(sample_t* data, size_t size) {
    roc_panic_if(!data);
    roc_panic_if(size == 0);

    memset(data, 0, size * sizeof(sample_t));

    for (IReader* rp = readers_.front(); rp; rp = readers_.nextof(*rp)) {
        sample_t* temp_data = temp_buf_.data();

        Frame temp_frame(temp_data, size);
        if (!rp->read(temp_frame)) {
            continue;
        }

        for (size_t n = 0; n < size; n++) {
            data[n] = clamp(data[n] + temp_data[n]);
        }
    }
}

} // namespace audio
} // namespace roc
