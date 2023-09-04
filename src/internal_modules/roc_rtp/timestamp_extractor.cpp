/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "timestamp_extractor.h"

namespace roc {
namespace rtp {

TimestampExtractor::TimestampExtractor(packet::IWriter& writer)
    : writer_(writer)
    , valid_(false)
    , capt_ts_(0)
    , rtp_ts_(0) {
}

TimestampExtractor::~TimestampExtractor() {
}

void TimestampExtractor::write(const packet::PacketPtr& pkt) {
    if (!pkt) {
        return;
    }

    if (pkt->rtp() && pkt->rtp()->capture_timestamp) {
        valid_ = true;
        capt_ts_ = pkt->rtp()->capture_timestamp;
        rtp_ts_ = pkt->rtp()->timestamp;
    }

    writer_.write(pkt);
}

bool TimestampExtractor::get_mapping(core::nanoseconds_t& ns,
                                     packet::timestamp_t& rtp) const {
    if (!valid_) {
        return false;
    }
    ns = capt_ts_;
    rtp = rtp_ts_;
    return true;
}

} // namespace rtp
} // namespace roc
