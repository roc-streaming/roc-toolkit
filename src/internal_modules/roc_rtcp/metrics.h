/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/metrics.h
//! @brief RTCP-derived metrics.

#ifndef ROC_RTCP_METRICS_H_
#define ROC_RTCP_METRICS_H_

#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace rtcp {

//! Metrics sent from sender to receiver.
struct SendingMetrics {
    //! Unix time when these metrics were generated.
    //! Nanoseconds since Unix epoch in sender clock domain.
    core::nanoseconds_t origin_time;

    //! RTP timestamp correspnding to NTP timestamp.
    packet::stream_timestamp_t origin_rtp;

    SendingMetrics()
        : origin_time(0)
        , origin_rtp(0) {
    }
};

//! Metrics sent from receiver to sender per source.
struct ReceptionMetrics {
    //! To which source these metrics apply.
    packet::stream_source_t ssrc;

    //! Fraction of lost packets.
    float fract_loss;

    ReceptionMetrics()
        : ssrc(0)
        , fract_loss(0) {
    }
};

//! Metrics for network link.
//! Calculated independently on both sender and receiver.
struct LinkMetrics {
    //! Estimated round-trip time.
    core::nanoseconds_t rtt;

    LinkMetrics()
        : rtt(0) {
    }
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_METRICS_H_
