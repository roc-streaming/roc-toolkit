/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

#include "roc_audio/mixer.h"

namespace roc {
namespace audio {

namespace {

packet::sample_t clamp(const packet::sample_t x) {
    if (x > packet::sample_max_val) {
        return packet::sample_max_val;
    } else if (x < packet::sample_min_val) {
        return packet::sample_min_val;
    } else {
        return x;
    }
}

} // anonymous

Mixer::Mixer(ISampleBufferComposer& composer) {
    if (!(temp_ = composer.compose())) {
        roc_panic("mixer: can't compose buffer in constructor");
    }
}

void Mixer::add(IStreamReader& reader) {
    readers_.append(reader);
}

void Mixer::remove(IStreamReader& reader) {
    readers_.remove(reader);
}

void Mixer::read(const ISampleBufferSlice& out) {
    packet::sample_t* out_data = out.data();
    size_t out_sz = out.size();

    if (out_data == NULL) {
        roc_panic("mixer: attempting to pass empty buffer");
    }

    if (out_sz == 0) {
        return;
    }

    memset(out_data, 0, out_sz * sizeof(packet::sample_t));

    temp_->set_size(out_sz);

    for (IStreamReader* reader = readers_.front(); reader;
         reader = readers_.next(*reader)) {
        reader->read(*temp_);

        roc_panic_if(temp_->size() != out_sz);

        packet::sample_t* temp_data = temp_->data();

        for (size_t n = 0; n < out_sz; n++) {
            out_data[n] = clamp(out_data[n] + temp_data[n]);
        }
    }
}

} // namespace audio
} // namespace roc
