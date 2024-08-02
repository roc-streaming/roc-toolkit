/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/timestamp_injector.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace rtp {

namespace {

const core::nanoseconds_t ReportInterval = core::Second * 30;

} // namespace

TimestampInjector::TimestampInjector(packet::IReader& reader,
                                     const audio::SampleSpec& sample_spec)
    : has_ts_(false)
    , capt_ts_(0)
    , rtp_ts_(0)
    , reader_(reader)
    , sample_spec_(sample_spec)
    , n_drops_(0)
    , rate_limiter_(ReportInterval) {
}

status::StatusCode TimestampInjector::init_status() const {
    return status::StatusOK;
}

status::StatusCode TimestampInjector::read(packet::PacketPtr& pkt,
                                           packet::PacketReadMode mode) {
    const status::StatusCode code = reader_.read(pkt, mode);
    if (code != status::StatusOK) {
        return code;
    }

    if (!pkt->has_flags(packet::Packet::FlagRTP)) {
        roc_panic("timestamp injector: unexpected non-rtp packet");
    }

    if (has_ts_) {
        const packet::stream_timestamp_diff_t rtp_dn =
            packet::stream_timestamp_diff(pkt->rtp()->stream_timestamp, rtp_ts_);

        pkt->rtp()->capture_timestamp =
            capt_ts_ + sample_spec_.stream_timestamp_delta_2_ns(rtp_dn);
    }

    return status::StatusOK;
}

void TimestampInjector::update_mapping(core::nanoseconds_t capture_ts,
                                       packet::stream_timestamp_t rtp_ts) {
    if (rate_limiter_.allow()) {
        roc_log(LogDebug,
                "timestamp injector: received mapping:"
                " old=cts:%lld/sts:%llu new=cts:%lld/sts:%llu has_ts=%d n_drops=%lu",
                (long long)capt_ts_, (unsigned long long)rtp_ts_, (long long)capture_ts,
                (unsigned long long)rtp_ts, (int)has_ts_, (unsigned long)n_drops_);
    }

    if (capture_ts <= 0) {
        roc_log(LogTrace, "timestamp injector: dropping mapping with negative cts");
        n_drops_++;
        return;
    }

    capt_ts_ = capture_ts;
    rtp_ts_ = rtp_ts;
    has_ts_ = true;
}

} // namespace rtp
} // namespace roc
