/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/poison_reader.h"
#include "roc_audio/sample.h"
#include "roc_core/memory_ops.h"

namespace roc {
namespace audio {

PoisonReader::PoisonReader(IFrameReader& reader)
    : reader_(reader) {
}

bool PoisonReader::read(Frame& frame) {
    core::MemoryOps::poison_before_use(frame.samples(),
                                       frame.num_samples() * sizeof(sample_t));

    return reader_.read(frame);
}

} // namespace audio
} // namespace roc
