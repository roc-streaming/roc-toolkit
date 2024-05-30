/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/udp.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace packet {

UDP::UDP()
    : receive_timestamp(0)
    , queue_timestamp(0) {
    memset(&request, 0, sizeof(request));
}

} // namespace packet
} // namespace roc
