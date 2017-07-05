/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/validator.h"
#include "roc_core/log.h"
#include "roc_core/macros.h"

namespace roc {
namespace rtp {

Validator::Validator(packet::IReader& reader,
                     const Format& format,
                     const ValidatorConfig& config)
    : format_(format)
    , reader_(reader)
    , config_(config) {
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

    packet::signed_seqnum_t sn_dist =
        ROC_UNSIGNED_SUB(packet::signed_seqnum_t, next.seqnum, prev.seqnum);

    sn_dist = ROC_ABS(sn_dist);

    if ((size_t)sn_dist > config_.max_sn_jump) {
        roc_log(LogDebug,
                "rtp validator: too long seqnum jump: prev=%lu next=%lu dist=%lu",
                (unsigned long)prev.seqnum, (unsigned long)next.seqnum,
                (unsigned long)sn_dist);
        return false;
    }

    packet::signed_timestamp_t ts_dist =
        ROC_UNSIGNED_SUB(packet::signed_timestamp_t, next.timestamp, prev.timestamp);

    ts_dist = ROC_ABS(ts_dist);

    size_t ts_dist_1k = (size_t)ts_dist * 1000;
    size_t ts_dist_ms = (size_t)ts_dist_1k / format_.sample_rate;

    if (ts_dist_1k < (size_t)ts_dist || ts_dist_ms > config_.max_ts_jump) {
        roc_log(LogDebug, "rtp validator:"
                          " too long timestamp jump: prev=%lu next=%lu dist=%lu,%lums",
                (unsigned long)prev.timestamp, (unsigned long)next.timestamp,
                (unsigned long)ts_dist, (unsigned long)ts_dist_ms);
        return false;
    }

    if (next.payload_type != format_.payload_type) {
        roc_log(LogDebug,
                "rtp validator: unexpected payload type: expected=%u, actual=%u",
                (unsigned)format_.payload_type, (unsigned)next.payload_type);
        return false;
    }

    return true;
}

} // namespace rtp
} // namespace roc
