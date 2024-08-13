/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_fec/iblock_encoder.h"

namespace roc {
namespace fec {

IBlockEncoder::IBlockEncoder(core::IArena& arena)
    : core::ArenaAllocation(arena) {
}

IBlockEncoder::~IBlockEncoder() {
}

} // namespace fec
} // namespace roc
