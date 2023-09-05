/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/timestamp_extractor.h"
#include "roc_core/panic.h"

namespace roc {
namespace rtp {

TimestampExtractor::TimestampExtractor(packet::IWriter& writer,
                                       const audio::SampleSpec& sample_spec)
    : writer_(writer)
    , has_ts_(false)
    , capt_ts_(0)
    , rtp_ts_(0)
    , sample_spec_(sample_spec) {
}

TimestampExtractor::~TimestampExtractor() {
}

void TimestampExtractor::write(const packet::PacketPtr& pkt) {
    if (!pkt) {
        roc_panic("timestamp extractor: unexpected null packet");
    }

    if (!pkt->rtp()) {
        roc_panic("timestamp extractor: unexpected non-rtp packet");
    }

    if (pkt->rtp()->capture_timestamp != 0) {
        has_ts_ = true;
        capt_ts_ = pkt->rtp()->capture_timestamp;
        rtp_ts_ = pkt->rtp()->timestamp;
    }

    writer_.write(pkt);
}

bool TimestampExtractor::get_mapping(core::nanoseconds_t capture_ts,
                                     packet::timestamp_t* rtp_ts) const {
    if (!rtp_ts) {
        roc_panic("timestamp extractor: unexpected null pointer");
    }

    if (!has_ts_) {
        return false;
    }

    const packet::timestamp_diff_t dn =
        sample_spec_.ns_2_rtp_timestamp(capture_ts - capt_ts_);

    *rtp_ts = rtp_ts_ + (packet::timestamp_t)dn;

    return true;
}

} // namespace rtp
} // namespace roc
