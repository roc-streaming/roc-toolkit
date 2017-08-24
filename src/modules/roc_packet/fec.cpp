/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/fec.h"
#include "roc_core/macros.h"
#include "roc_core/panic.h"

namespace roc {
namespace packet {

FEC::FEC()
    : source_blknum(0)
    , repair_blknum(0) {
}

int FEC::compare(const FEC& other) const {
    // TODO
    (void)other;
    roc_panic("not implemented");
}

} // namespace packet
} // namespace roc
