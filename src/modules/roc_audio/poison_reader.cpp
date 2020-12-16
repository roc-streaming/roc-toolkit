/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/poison_reader.h"
#include "roc_audio/units.h"

namespace roc {
namespace audio {

PoisonReader::PoisonReader(IReader& reader)
    : reader_(reader) {
}

ssize_t PoisonReader::read(Frame& frame) {
    const size_t frame_size = frame.size();
    sample_t* frame_data = frame.data();

    for (size_t n = 0; n < frame_size; n++) {
        frame_data[n] = SampleMax;
    }

    return reader_.read(frame);
}

} // namespace audio
} // namespace roc
