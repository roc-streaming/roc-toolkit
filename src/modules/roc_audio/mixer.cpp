/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
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

Mixer::Mixer(core::BufferPool<sample_t>& buffer_pool) {
    temp_buf_ = new (buffer_pool) core::Buffer<sample_t>(buffer_pool);
    if (!temp_buf_) {
        roc_log(LogError, "mixer: can't allocate temporary buffer");
        return;
    }
}

bool Mixer::valid() const {
    return temp_buf_;
}

void Mixer::add(IReader& reader) {
    readers_.push_back(reader);
}

void Mixer::remove(IReader& reader) {
    readers_.remove(reader);
}

void Mixer::read(Frame& frame) {
    roc_panic_if(!valid());

    const size_t max_read = temp_buf_.capacity();

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
}

void Mixer::read_(sample_t *out_data, size_t out_sz) {
    roc_panic_if_not(out_data);

    temp_buf_.resize(out_sz);
    memset(out_data, 0, out_sz * sizeof(sample_t));

    for (IReader* rp = readers_.front(); rp; rp = readers_.nextof(*rp)) {
        Frame temp(temp_buf_.data(), temp_buf_.size());
        rp->read(temp);

        const sample_t* temp_data = temp.data();
        for (size_t n = 0; n < out_sz; n++) {
            out_data[n] = clamp(out_data[n] + temp_data[n]);
        }
    }
}

} // namespace audio
} // namespace roc
