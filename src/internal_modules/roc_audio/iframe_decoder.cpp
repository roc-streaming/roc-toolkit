/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/iframe_decoder.h"

namespace roc {
namespace audio {

IFrameDecoder::IFrameDecoder(core::IArena& arena)
    : core::ArenaAllocation(arena) {
}

IFrameDecoder::~IFrameDecoder() {
}

} // namespace audio
} // namespace roc
