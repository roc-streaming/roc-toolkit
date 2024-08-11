/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/ilink_meter.h
//! @brief Link meter interface.

#ifndef ROC_PACKET_ILINK_METER_H_
#define ROC_PACKET_ILINK_METER_H_

#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace packet {

//! Link metrics.
struct LinkMetrics {
    //! Extended lowest RTP seqnum received.
    //! The low 16 bits contain the lowest sequence number received in an RTP data
    //! packet, and the rest bits extend that sequence number with the corresponding
    //! count of seqnum cycles.
    //! @note
    //!  Available on both receiver and sender.
    //!  Calculated by rtp::LinkMeter on receiver, reported via RTCP to sender.
    packet::ext_seqnum_t ext_first_seqnum;

    //! Extended highest RTP seqnum received.
    //! The low 16 bits contain the highest sequence number received in an RTP data
    //! packet, and the rest bits extend that sequence number with the corresponding
    //! count of seqnum cycles.
    //! @note
    //!  Available on both receiver and sender.
    //!  Calculated by rtp::LinkMeter on receiver, reported via RTCP to sender.
    packet::ext_seqnum_t ext_last_seqnum;

    //! Total amount of packets that receiver expects to be delivered.
    //! Calculated based on seqnums of oldest and newest packets.
    //! @note
    //!  Available on both receiver and sender.
    //!  Calculated by rtp::LinkMeter on receiver, reported via RTCP to sender.
    uint64_t expected_packets;

    //! Cumulative count of lost packets.
    //! The total number of RTP data packets that have been lost since the beginning
    //! of reception. Defined to be the number of packets expected minus the number of
    //! packets actually received, where the number of packets received includes any
    //! which are late or duplicates. Packets that arrive late are not counted as lost,
    //! and the loss may be negative if there are duplicates.
    //! @note
    //!  Available on both receiver and sender.
    //!  Calculated by rtp::LinkMeter on receiver, reported via RTCP to sender.
    int64_t lost_packets;

    //! Average interarrival jitter.
    //! An estimate of the statistical variance of the RTP data packet interarrival time.
    //! Calculated based on a sliding window.
    //! @note
    //!  This value is calculated on sliding window on a receiver side and sender
    //!  side gets this value via RTCP.
    core::nanoseconds_t mean_jitter;

    //! Peak interarrival jitter.
    //! An estimate of the maximum jitter, excluding short small spikes.
    //! Calculated based on a sliding window.
    //! @note
    //!  Available only on receiver.
    //!  Calculated by rtp::LinkMeter.
    core::nanoseconds_t peak_jitter;

    //! Estimated round-trip time between sender and receiver.
    //! Calculated based on NTP-like timestamp exchange implemented by RTCP protocol.
    //! @note
    //!  Available on both receiver and sender.
    //!  Calculated by rtcp::Communicator independently on receiver and sender.
    core::nanoseconds_t rtt;

    LinkMetrics()
        : ext_first_seqnum(0)
        , ext_last_seqnum(0)
        , expected_packets(0)
        , lost_packets(0)
        , mean_jitter(0)
        , peak_jitter(0)
        , rtt(0) {
    }
};

//! Link meter interface.
class ILinkMeter {
public:
    virtual ~ILinkMeter();

    //! Check if metrics are available.
    virtual bool has_metrics() const = 0;

    //! Get metrics.
    virtual const LinkMetrics& metrics() const = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_ILINK_METER_H_
