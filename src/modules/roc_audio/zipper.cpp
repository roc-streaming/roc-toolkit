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

#include "roc_audio/zipper.h"

namespace roc {
namespace audio {

Zipper::Zipper(ISampleBufferComposer& composer) {
    if (!(temp_ = composer.compose())) {
        roc_panic("zipper: can't compose buffer in constructor");
    }
}

void Zipper::add(IStreamReader& reader) {
    readers_.append(reader);
}

void Zipper::remove(IStreamReader& reader) {
    readers_.remove(reader);
}

void Zipper::read(const ISampleBufferSlice& out) {
    size_t num_readers = readers_.size();

    packet::sample_t* out_data = out.data();
    size_t out_sz = out.size();

    if (out_data == NULL) {
        roc_panic("zipper: attempting to pass empty buffer");
    }

    if (num_readers == 0) {
        if (out_sz > 0) {
            memset(out_data, 0, out_sz * sizeof(packet::sample_t));
        }
        return;
    }

    if (out_sz % num_readers != 0) {
        roc_panic("zipper: attempting to read number of samples which is "
                  "not multiple of number of readers "
                  "(num_samples=%u, num_readers=%u)",
                  (unsigned)out_sz, (unsigned)num_readers);
    }

    size_t temp_sz = (out_sz / num_readers);

    temp_->set_size(temp_sz);

    size_t cur_reader = 0;

    for (IStreamReader* reader = readers_.front(); reader;
         reader = readers_.next(*reader)) {
        reader->read(*temp_);

        roc_panic_if(temp_->size() != temp_sz);

        packet::sample_t* temp_data = temp_->data();

        for (size_t n = 0; n < temp_sz; n++) {
            out_data[n * num_readers + cur_reader] = temp_data[n];
        }

        cur_reader++;
    }
}

} // namespace audio
} // namespace roc
