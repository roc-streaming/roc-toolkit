/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/iparser.h"

namespace roc {
namespace packet {

IParser::IParser(core::IArena& arena)
    : core::ArenaAllocation(arena) {
}

IParser::~IParser() {
}

} // namespace packet
} // namespace roc
