/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/reports.h
//! @brief RTCP reports.

#ifndef ROC_RTCP_REPORTS_H_
#define ROC_RTCP_REPORTS_H_

#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace rtcp {

//! Sender report, for inspection on receiver.
//! @remarks
//!  This struct accumulates data of SDES, SR, and XR packets.
//!  On sender, it's queried from pipeline and used to generate RTCP packets.
//!  On receiver, it's filled from RTCP packets and passed to pipeline.
struct SendReport {
    //! CNAME of sender.
    //! Should not change.
    //! On sender, should be same as local CNAME.
    const char* sender_cname;

    //! SSRC of sender.
    //! Should not change.
    //! On sender, should be same as local SSRC.
    packet::stream_source_t sender_source_id;

    //! Absolute timestamp of report in nanoseconds since Unix epoch.
    //! Defines time when report was sent in sender's clock domain.
    core::nanoseconds_t report_timestamp;

    //! RTP timestamp corresponding to absolute timestamp.
    //! Estimated stream timestamp (STS) of the sample being captured at the time
    //! corresponding to report_timestamp.
    packet::stream_timestamp_t stream_timestamp;

    //! Number of packets sent.
    //! The total number of RTP data packets transmitted by the sender since starting
    //! transmission up until the time of this report.
    uint32_t packet_count;

    //! Number of bytes sent.
    //! The total number of payload octets (i.e., not including header or padding)
    //! transmitted in RTP data packets by the sender since starting transmission
    //! up until the time this report.
    uint32_t byte_count;

    SendReport()
        : sender_cname(NULL)
        , sender_source_id(0)
        , report_timestamp(0)
        , stream_timestamp(0)
        , packet_count(0)
        , byte_count(0) {
    }
};

//! Receiver report, for inspection on sender.
//! @remarks
//!  This struct accumulates data of SDES, RR and XR packets.
//!  On receiver, it's queried from pipeline and used to generate RTCP packets.
//!  On sender, it's filled from RTCP packets and passed to pipeline.
struct RecvReport {
    //! CNAME of receiver.
    //! Should not change.
    //! On receiver, should be same as local CNAME.
    const char* receiver_cname;

    //! SSRC of receiver.
    //! Should not change.
    //! On receiver, should be same as local SSRC.
    packet::stream_source_t receiver_source_id;

    //! SSRC of sender.
    //! Should not change.
    packet::stream_source_t sender_source_id;

    //! Absolute timestamp of report in nanoseconds since Unix epoch.
    //! Defines time when report was sent in receiver's clock domain.
    core::nanoseconds_t report_timestamp;

    //! Extended highest sequence number received.
    //! The low 16 bits contain the highest sequence number received in an RTP data
    //! packet, and the high 16 bits extend that sequence number with the corresponding
    //! count of sequence number cycles.
    uint32_t ext_last_seqnum;

    //! Fraction of lost packets from 0 to 1.
    //! The fraction of RTP data packets lost since the previous report was sent.
    //! Defined to be the number of packets lost divided by the number of packets
    //! expected. If the loss is negative due to duplicates, set to zero.
    float fract_loss;

    //! Cumulative count of lost packets.
    //! The total number of RTP data packets that have been lost since the beginning
    //! of reception. Defined to be the number of packets expected minus the number of
    //! packets actually received, where the number of packets received includes any
    //! which are late or duplicates. Packets that arrive late are not counted as lost,
    //! and the loss may be negative if there are duplicates.
    int32_t cum_loss;

    //! Estimated interarrival jitter, in timestamp units.
    //! An estimate of the statistical variance of the RTP data packet interarrival
    //! time, measured in timestamp units.
    packet::stream_timestamp_t jitter;

    RecvReport()
        : receiver_cname(NULL)
        , receiver_source_id(0)
        , sender_source_id(0)
        , report_timestamp(0)
        , ext_last_seqnum(0)
        , fract_loss(0)
        , cum_loss(0)
        , jitter(0) {
    }
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_REPORTS_H_
