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

Mixer::Mixer(core::BufferPool<sample_t>& buffer_pool)
    : buffer_pool_(buffer_pool) {
}

void Mixer::read(Frame& frame) {
    if (!temp_.samples) {
        temp_.samples = new (buffer_pool_) core::Buffer<sample_t>(buffer_pool_);
        if (!temp_.samples) {
            roc_log(LogError, "mixer: can't allocate temporary buffer");
            return;
        }
    }

    const size_t out_sz = frame.samples.size();
    if (out_sz == 0) {
        return;
    }

    sample_t* out_data = frame.samples.data();
    if (!out_data) {
        roc_panic("mixer: null data");
    }

    temp_.samples.resize(out_sz);
    memset(out_data, 0, out_sz * sizeof(sample_t));

    for (IReader* rp = readers_.front(); rp; rp = readers_.nextof(*rp)) {
        rp->read(temp_);
        roc_panic_if(temp_.samples.size() != out_sz);

        const sample_t* temp_data = temp_.samples.data();

        for (size_t n = 0; n < out_sz; n++) {
            out_data[n] = clamp(out_data[n] + temp_data[n]);
        }
    }
}

void Mixer::add(IReader& reader) {
    readers_.push_back(reader);
}

void Mixer::remove(IReader& reader) {
    readers_.remove(reader);
}

} // namespace audio
} // namespace roc
