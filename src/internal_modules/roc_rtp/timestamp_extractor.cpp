/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/timestamp_extractor.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace rtp {

namespace {

const core::nanoseconds_t ReportInterval = core::Second * 30;

} // namespace

TimestampExtractor::TimestampExtractor(packet::IWriter& writer,
                                       const audio::SampleSpec& sample_spec)
    : writer_(writer)
    , has_ts_(false)
    , capt_ts_(0)
    , rtp_ts_(0)
    , sample_spec_(sample_spec)
    , rate_limiter_(ReportInterval) {
}

status::StatusCode TimestampExtractor::init_status() const {
    return status::StatusOK;
}

status::StatusCode TimestampExtractor::write(const packet::PacketPtr& pkt) {
    if (!pkt) {
        roc_panic("timestamp extractor: unexpected null packet");
    }

    if (!pkt->has_flags(packet::Packet::FlagRTP)) {
        roc_panic("timestamp extractor: unexpected non-rtp packet");
    }

    if (pkt->rtp()->capture_timestamp < 0) {
        roc_panic("timestamp extractor: unexpected negative cts in packet: cts=%lld",
                  (long long)pkt->rtp()->capture_timestamp);
    }

    if (pkt->rtp()->capture_timestamp != 0) {
        has_ts_ = true;
        capt_ts_ = pkt->rtp()->capture_timestamp;
        rtp_ts_ = pkt->rtp()->stream_timestamp;
    }

    return writer_.write(pkt);
}

bool TimestampExtractor::has_mapping() {
    return has_ts_;
}

packet::stream_timestamp_t
TimestampExtractor::get_mapping(core::nanoseconds_t capture_ts) {
    if (capture_ts <= 0) {
        roc_panic(
            "timestamp extractor: unexpected negative cts in mapping request: cts=%lld",
            (long long)capture_ts);
    }

    if (!has_ts_) {
        roc_panic(
            "timestamp extractor: attempt to get mapping before it becomes available");
    }

    const packet::stream_timestamp_diff_t dn =
        sample_spec_.ns_2_stream_timestamp_delta(capture_ts - capt_ts_);

    const packet::stream_timestamp_t rtp_ts = rtp_ts_ + (packet::stream_timestamp_t)dn;

    if (rate_limiter_.allow()) {
        roc_log(LogDebug, "timestamp extractor: returning mapping: cts:%lld/sts:%llu",
                (long long)capture_ts, (unsigned long long)rtp_ts);
    }

    return rtp_ts;
}

} // namespace rtp
} // namespace roc
