/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/timestamp_injector.h"

namespace roc {
namespace rtp {

TimestampInjector::TimestampInjector(packet::IReader& packet_src,
                                 const audio::SampleSpec& sample_spec)
    : valid_ts_(false)
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

    if (pkt->rtp() && valid_ts_) {
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
    ts_ = capture_ts;
    rtp_ts_ = rtp_ts;
    valid_ts_ = !!capture_ts;
}

} // namespace rtp
} // namespace roc
