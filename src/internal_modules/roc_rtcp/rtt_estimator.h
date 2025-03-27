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
#include "roc_dbgio/csv_dumper.h"
#include "roc_packet/units.h"
#include "roc_stat/mov_quantile.h"

namespace roc {
namespace rtcp {

//! Round-trip time metrics.
struct RttConfig {
    //! Measurement interval duration.
    //! All metrics below are computed for a sliding window of this length.
    core::nanoseconds_t interval_duration;

    //! How many measurements of RTT is used
    //! to find median value.
    size_t rtt_winlen;

    //! How many measurements of RTT is used
    //! to find median value.
    size_t clock_offset_winlen;

    RttConfig()
        : interval_duration(core::Second * 5)
        , rtt_winlen(15)
        , clock_offset_winlen(100) {
    }
};

//! Round-trip time metrics.
struct RttMetrics {
    //! Estimated offset of remote clock relative to local clock.
    //! Estimated based on RTT and local/remote timestamp.
    core::nanoseconds_t clock_offset;

    //! Estimated round-trip time.
    core::nanoseconds_t rtt;

    RttMetrics()
        : clock_offset(0)
        , rtt(0) {
    }
};

//! Round-trip time estimator.
//! Created inside rtcp::Reporter for each RTP stream.
//! Continously computes RTT and clock offset based on LSR/DLSR
//! or LRR/DLRR timestamps.
class RttEstimator {
public:
    //! Initialize.
    RttEstimator(const RttConfig& config, core::IArena& arena, dbgio::CsvDumper* dumper);

    //! Check whether metrics are already available.
    bool has_metrics() const;

    //! Get estimated metrics.
    const RttMetrics& metrics() const;

    //! Update metrics with new data.
    //! Parameters:
    //!  - @p local_report_ts - local unix time when we've sent report
    //!  - @p remote_report_ts - remote unix time when they've received our report
    //!  - @p remote_reply_ts - remote unix time when they've send reply report
    //!  - @p local_reply_ts - local unix time when we've received their reply
    void update(core::nanoseconds_t local_report_ts,
                core::nanoseconds_t remote_report_ts,
                core::nanoseconds_t remote_reply_ts,
                core::nanoseconds_t local_reply_ts);

private:
    void dump_(core::nanoseconds_t local_report_ts,
               core::nanoseconds_t remote_report_ts,
               core::nanoseconds_t remote_reply_ts,
               core::nanoseconds_t local_reply_ts);

    const RttConfig config_;
    RttMetrics metrics_;
    bool has_metrics_;

    core::nanoseconds_t first_report_ts_;
    core::nanoseconds_t last_report_ts_;

    dbgio::CsvDumper* dumper_;
    stat::MovQuantile<core::nanoseconds_t> rtt_stats_;
    stat::MovQuantile<core::nanoseconds_t> clock_offset_stats_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_RTT_ESTIMATOR_H_
