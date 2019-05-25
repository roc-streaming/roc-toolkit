/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/validator.h"
#include "roc_core/log.h"

namespace roc {
namespace rtp {

Validator::Validator(packet::IReader& reader,
                     const ValidatorConfig& config,
                     size_t sample_rate)
    : reader_(reader)
    , config_(config)
    , sample_rate_(sample_rate) {
}

packet::PacketPtr Validator::read() {
    packet::PacketPtr next_packet = reader_.read();
    if (!next_packet) {
        return NULL;
    }

    const packet::RTP* next_rtp = next_packet->rtp();
    if (!next_rtp) {
        roc_log(LogDebug, "rtp validator: unexpected non-RTP packet");
        return NULL;
    }

    const packet::RTP* prev_rtp = NULL;
    if (prev_packet_) {
        prev_rtp = prev_packet_->rtp();
    }

    if (prev_rtp && !check_(*prev_rtp, *next_rtp)) {
        return NULL;
    }

    if (!prev_rtp || prev_rtp->compare(*next_rtp) < 0) {
        prev_packet_ = next_packet;
    }

    return next_packet;
}

bool Validator::check_(const packet::RTP& prev, const packet::RTP& next) const {
    if (prev.source != next.source) {
        roc_log(LogDebug, "rtp validator: source id jump: prev=%lu next=%lu",
                (unsigned long)prev.source, (unsigned long)next.source);
        return false;
    }

    if (next.payload_type != prev.payload_type) {
        roc_log(LogDebug, "rtp validator: payload type jump: prev=%u, next=%u",
                (unsigned)prev.payload_type, (unsigned)next.payload_type);
        return false;
    }

    packet::seqnum_diff_t sn_dist = packet::seqnum_diff(next.seqnum, prev.seqnum);
    if (sn_dist < 0) {
        sn_dist = -sn_dist;
    }

    if ((size_t)sn_dist > config_.max_sn_jump) {
        roc_log(LogDebug,
                "rtp validator: too long seqnum jump: prev=%lu next=%lu dist=%lu",
                (unsigned long)prev.seqnum, (unsigned long)next.seqnum,
                (unsigned long)sn_dist);
        return false;
    }

    packet::timestamp_diff_t ts_dist =
        packet::timestamp_diff(next.timestamp, prev.timestamp);
    if (ts_dist < 0) {
        ts_dist = -ts_dist;
    }

    const core::nanoseconds_t ts_dist_ns = packet::timestamp_to_ns(ts_dist, sample_rate_);

    if (ts_dist_ns > config_.max_ts_jump) {
        roc_log(LogDebug,
                "rtp validator:"
                " too long timestamp jump: prev=%lu next=%lu dist=%lu",
                (unsigned long)prev.timestamp, (unsigned long)next.timestamp,
                (unsigned long)ts_dist);
        return false;
    }

    return true;
}

} // namespace rtp
} // namespace roc
