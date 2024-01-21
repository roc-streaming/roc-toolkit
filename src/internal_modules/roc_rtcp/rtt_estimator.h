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
#include "roc_packet/units.h"

namespace roc {
namespace rtcp {

//! Round-trip time metrics.
struct RttConfig {
    //! Measurement interval duration.
    //! All metrics below are computed for a sliding window of this length.
    core::nanoseconds_t interval_duration;

    RttConfig()
        : interval_duration(core::Second * 5) {
    }
};

//! Round-trip time metrics.
struct RttMetrics {
    //! Measurement cumulative duration.
    //! Total duration from the very beginning.
    core::nanoseconds_t cumulative_duration;

    //! Measurement interval duration.
    //! All metrics below are computed for a sliding window of this length.
    core::nanoseconds_t interval_duration;

    //! Extended seqnums of first packet in current measurement interval.
    packet::ext_seqnum_t interval_first_seqnum;

    //! Extended seqnums of last packet in current measurement interval.
    packet::ext_seqnum_t interval_last_seqnum;

    //! Estimated offset of remote clock relative to local clock.
    //! Estimated based on RTT and local/remote timestamp.
    core::nanoseconds_t clock_offset;

    //! Estimated round-trip time (latest in interval).
    core::nanoseconds_t rtt_last;

    //! Estimated round-trip time (average during interval).
    core::nanoseconds_t rtt_avg;

    //! Estimated round-trip time (minimum during interval).
    core::nanoseconds_t rtt_min;

    //! Estimated round-trip time (maximum during interval).
    core::nanoseconds_t rtt_max;

    RttMetrics()
        : cumulative_duration(0)
        , interval_duration(0)
        , interval_first_seqnum(0)
        , interval_last_seqnum(0)
        , clock_offset(0)
        , rtt_last(0)
        , rtt_avg(0)
        , rtt_min(0)
        , rtt_max(0) {
    }
};

//! Round-trip time estimator.
//! Created inside rtcp::Reporter for each RTP stream.
//! Continously computes RTT and clock offset based on LSR/DLSR
//! or LRR/DLRR timestamps.
class RttEstimator {
public:
    //! Initialize.
    RttEstimator(const RttConfig& config);

    //! Get estimated metrics.
    const RttMetrics& metrics() const;

    //! Update metrics with new data.
    //! Parameters:
    //!  - @p last_seqnum - extended seqnum of latest packet in RTP stream
    //!  - @p local_report_ts - local unix time when we've sent report
    //!  - @p remote_report_ts - remote unix time when they've received our report
    //!  - @p remote_reply_ts - remote unix time when they've send reply report
    //!  - @p local_reply_ts - local unix time when we've received their reply
    void update(packet::ext_seqnum_t last_seqnum,
                core::nanoseconds_t local_report_ts,
                core::nanoseconds_t remote_report_ts,
                core::nanoseconds_t remote_reply_ts,
                core::nanoseconds_t local_reply_ts);

private:
    const RttConfig config_;
    RttMetrics metrics_;

    core::nanoseconds_t first_report_ts_;
    core::nanoseconds_t last_report_ts_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_RTT_ESTIMATOR_H_
