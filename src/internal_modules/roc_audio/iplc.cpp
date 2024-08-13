/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/iplc.h"

namespace roc {
namespace audio {

IPlc::IPlc(core::IArena& arena)
    : core::ArenaAllocation(arena) {
}

IPlc::~IPlc() {
}

} // namespace audio
} // namespace roc
