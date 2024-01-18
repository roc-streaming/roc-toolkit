/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/rtt_estimator.h"
#include "roc_core/log.h"

namespace roc {
namespace rtcp {

RttEstimator::RttEstimator()
    : last_local_report_ts_(0)
    , clock_offset_(0)
    , rtt_(0) {
}

core::nanoseconds_t RttEstimator::clock_offset() const {
    return clock_offset_;
}

core::nanoseconds_t RttEstimator::last_rtt() const {
    return rtt_;
}

// Notation:
//   T1 (local_report_ts)  = local timestamp upon report departure
//   T2 (remote_report_ts) = remote timestamp upon report arrival
//   T3 (remote_reply_ts)  = remote timestamp upon reply report departure
//   T4 (local_reply_ts)   = local timestamp upon reply report arrival
//
// Mapping to RTCP when we're sender:
//   T1 = LSR (when we sent SR)
//   T2 = RRTR_NTP-DLSR (when remote received SR)
//   T3 = RRTR_NTP (when remote sent RR)
//   T4 = LRR (when we received RR)
//
// Mapping to RTCP when we're receiver:
//   T1 = LRR (when we sent RR)
//   T2 = SR_NTP-DLRR (when remote received RR)
//   T3 = SR_NTP (when remote sent SR)
//   T4 = LSR (when we received SR)
//
// This mapping is implemented by rtcp::Reporter. RttEstimator doesn't
// need to know whether it's working on sender or receiver.
//
// See RFC 3550 and RFC 5905.
// See also https://www.eecis.udel.edu/~mills/time.html
void RttEstimator::update(core::nanoseconds_t local_report_ts,
                          core::nanoseconds_t remote_report_ts,
                          core::nanoseconds_t remote_reply_ts,
                          core::nanoseconds_t local_reply_ts) {
    if (!(local_report_ts <= local_reply_ts) || !(remote_report_ts <= remote_reply_ts)) {
        // Filter out obviously incorrect reports.
        return;
    }

    if (local_report_ts <= last_local_report_ts_) {
        // Filter out outdated reports.
        return;
    }

    // From RFC:
    //   offset = ((T2 - T1) + (T3 - T4)) / 2
    const core::nanoseconds_t clock_offset =
        ((remote_report_ts - local_report_ts) + (remote_reply_ts - local_reply_ts)) / 2;

    // From RFC:
    //   delay = ((T4 - T1) - (T3 - T2))
    const core::nanoseconds_t rtt =
        (local_reply_ts - local_report_ts) - (remote_reply_ts - remote_report_ts);

    if (rtt < 0) {
        // Filter out obviously incorrect results.
        return;
    }

    last_local_report_ts_ = local_report_ts;
    clock_offset_ = clock_offset;
    rtt_ = rtt;
}

} // namespace rtcp
} // namespace roc
