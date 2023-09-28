/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/rtp.h"

namespace roc {
namespace packet {

RTP::RTP()
    : source(0)
    , seqnum(0)
    , stream_timestamp(0)
    , duration(0)
    , capture_timestamp(0)
    , marker(false)
    , payload_type(0) {
}

int RTP::compare(const RTP& other) const {
    if (seqnum_lt(seqnum, other.seqnum)) {
        return -1;
    } else if (seqnum == other.seqnum) {
        return 0;
    } else {
        return 1;
    }
}

} // namespace packet
} // namespace roc
