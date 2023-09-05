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

namespace roc {
namespace rtp {

TimestampInjector::TimestampInjector(packet::IReader& packet_src,
                                     const audio::SampleSpec& sample_spec)
    : has_ts_(false)
    , ts_(0)
    , rtp_ts_(0)
    , reader_(packet_src)
    , sample_spec_(sample_spec) {
}

TimestampInjector::~TimestampInjector() {
}

packet::PacketPtr TimestampInjector::read() {
    packet::PacketPtr pkt = reader_.read();
    if (!pkt) {
        return NULL;
    }

    if (!pkt->rtp()) {
        roc_panic("timestamp injector: unexpected non-rtp packet");
    }

    if (pkt->rtp()->capture_timestamp != 0) {
        roc_panic(
            "timestamp injector: unexpected packet with non-zero capture timestamp");
    }

    if (has_ts_) {
        const packet::timestamp_diff_t dn =
            packet::timestamp_diff(pkt->rtp()->timestamp, rtp_ts_);

        if (dn >= 0) {
            pkt->rtp()->capture_timestamp =
                ts_ + sample_spec_.samples_per_chan_2_ns((size_t)dn);
        } else {
            pkt->rtp()->capture_timestamp =
                ts_ - sample_spec_.samples_per_chan_2_ns((size_t)-dn);
        }
    }

    return pkt;
}

void TimestampInjector::update_mapping(core::nanoseconds_t capture_ts,
                                       packet::timestamp_t rtp_ts) {
    if (capture_ts != 0) {
        if (!has_ts_) {
            roc_log(LogDebug, "timestamp injector: received first mapping");
        }
        ts_ = capture_ts;
        rtp_ts_ = rtp_ts;
        has_ts_ = true;
    }
}

} // namespace rtp
} // namespace roc
