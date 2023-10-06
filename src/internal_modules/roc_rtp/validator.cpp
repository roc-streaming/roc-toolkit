/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/validator.h"
#include "roc_core/log.h"
#include "roc_status/status_code.h"

namespace roc {
namespace rtp {

Validator::Validator(packet::IReader& reader,
                     const ValidatorConfig& config,
                     const audio::SampleSpec& sample_spec)
    : reader_(reader)
    , has_prev_packet_(false)
    , config_(config)
    , sample_spec_(sample_spec) {
}

status::StatusCode Validator::read(packet::PacketPtr& pp) {
    packet::PacketPtr next_packet;
    const status::StatusCode code = reader_.read(next_packet);
    if (code != status::StatusOK) {
        return code;
    }

    if (!next_packet->rtp()) {
        roc_log(LogDebug, "rtp validator: unexpected non-rtp packet");
        // TODO: return StatusAgain (gh-183)
        return status::StatusNoData;
    }

    if (has_prev_packet_ && !validate_(prev_packet_rtp_, *next_packet->rtp())) {
        // TODO: return StatusAgain (gh-183)
        return status::StatusNoData;
    }

    pp = next_packet;

    if (!has_prev_packet_ || prev_packet_rtp_.compare(*pp->rtp()) < 0) {
        has_prev_packet_ = true;
        prev_packet_rtp_ = *pp->rtp();
    }

    return status::StatusOK;
}

bool Validator::validate_(const packet::RTP& prev, const packet::RTP& next) const {
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

    packet::stream_timestamp_diff_t ts_dist =
        packet::stream_timestamp_diff(next.stream_timestamp, prev.stream_timestamp);
    if (ts_dist < 0) {
        ts_dist = -ts_dist;
    }

    const core::nanoseconds_t ts_dist_ns =
        sample_spec_.stream_timestamp_delta_2_ns(ts_dist);

    if (ts_dist_ns > config_.max_ts_jump) {
        roc_log(LogDebug,
                "rtp validator:"
                " too long timestamp jump: prev=%lu next=%lu dist=%lu",
                (unsigned long)prev.stream_timestamp,
                (unsigned long)next.stream_timestamp, (unsigned long)ts_dist);
        return false;
    }

    if (next.capture_timestamp < 0) {
        roc_log(LogDebug,
                "rtp validator:"
                " invalid negative cts: prev=%lld next=%lld",
                (long long)prev.capture_timestamp, (long long)next.capture_timestamp);
        return false;
    }

    if (next.capture_timestamp == 0 && prev.capture_timestamp != 0) {
        roc_log(LogDebug,
                "rtp validator:"
                " invalid zero cts after non-zero cts: prev=%lld next=%lld",
                (long long)prev.capture_timestamp, (long long)next.capture_timestamp);
        return false;
    }

    return true;
}

} // namespace rtp
} // namespace roc
