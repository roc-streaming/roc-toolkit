/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/rtt_estimator.h
//! @brief Round-trip time estimator.

#ifndef ROC_RTCP_RTT_ESTIMATOR_H_
#define ROC_RTCP_RTT_ESTIMATOR_H_

#include "roc_core/time.h"

namespace roc {
namespace rtcp {

//! Round-trip time estimator.
//! Created inside rtcp::Reporter for each RTP stream.
//! Continously computes RTT and clock offset based on LSR/DLSR
//! or LRR/DLRR timestamps.
class RttEstimator {
public:
    //! Initialize.
    RttEstimator();

    //! Get estimated offset of remote clock relative to local clock.
    core::nanoseconds_t clock_offset() const;

    //! Get most recent estimated round-trip time.
    core::nanoseconds_t last_rtt() const;

    //! Update values with new data.
    void update(core::nanoseconds_t local_report_ts,
                core::nanoseconds_t remote_report_ts,
                core::nanoseconds_t remote_reply_ts,
                core::nanoseconds_t local_reply_ts);

private:
    core::nanoseconds_t last_local_report_ts_;
    core::nanoseconds_t clock_offset_;
    core::nanoseconds_t rtt_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_RTT_ESTIMATOR_H_
