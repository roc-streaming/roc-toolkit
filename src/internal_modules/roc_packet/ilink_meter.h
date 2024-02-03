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
    packet::ext_seqnum_t ext_first_seqnum;

    //! Extended highest RTP seqnum received.
    //! The low 16 bits contain the highest sequence number received in an RTP data
    //! packet, and the rest bits extend that sequence number with the corresponding
    //! count of seqnum cycles.
    packet::ext_seqnum_t ext_last_seqnum;

    //! Total amount of packets sent or expected to be received.
    //! On sender, this counter is just incremented every packet.
    //! On receiver, it is derived from seqnums.
    uint64_t total_packets;

    //! Cumulative count of lost packets.
    //! The total number of RTP data packets that have been lost since the beginning
    //! of reception. Defined to be the number of packets expected minus the number of
    //! packets actually received, where the number of packets received includes any
    //! which are late or duplicates. Packets that arrive late are not counted as lost,
    //! and the loss may be negative if there are duplicates.
    int64_t cum_lost_packets;

    //! Fraction of lost packets from 0 to 1.
    //! The fraction of RTP data packets lost since the previous report was sent.
    //! Defined to be the number of packets lost divided by the number of packets
    //! expected. If the loss is negative due to duplicates, set to zero.
    float fract_lost_packets;

    //! Estimated interarrival jitter.
    //! An estimate of the statistical variance of the RTP data packet
    //! interarrival time.
    core::nanoseconds_t jitter;

    //! Estimated round-trip time between sender and receiver.
    //! Computed based on NTP-like timestamp exchange implemennted by RTCP protocol.
    //! Read-only field. You can read it on sender, but you should not set
    //! it on receiver.
    core::nanoseconds_t rtt;

    LinkMetrics()
        : ext_first_seqnum(0)
        , ext_last_seqnum(0)
        , total_packets(0)
        , cum_lost_packets(0)
        , fract_lost_packets(0)
        , jitter(0)
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
