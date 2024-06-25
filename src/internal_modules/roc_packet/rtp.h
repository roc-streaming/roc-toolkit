/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/rtp.h
//! @brief RTP packet.

#ifndef ROC_PACKET_RTP_H_
#define ROC_PACKET_RTP_H_

#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace packet {

//! RTP packet.
struct RTP {
    //! Packet source ID identifying packet stream ("ssrc").
    //! @remarks
    //!  Sequence numbers and timestamp are numbered independently inside
    //!  different packet streams.
    stream_source_t source_id;

    //! Packet sequence number in packet stream ("sn").
    //! @remarks
    //!  Packets are numbered sequentially in every stream, starting from some
    //!  random value. May overflow.
    seqnum_t seqnum;

    //! Packet stream timestamp ("sts").
    //! @remarks
    //!  Describes position of the first sample using abstract stream clock.
    //!  This clock belongs to sender and has sample rate of the stream.
    //!  For example, if sender is 44100Hz audio card, then stream timestamp
    //!  is incremented by one each generated sample, and it happens 44100
    //!  times per second, according to audio card clock.
    //!  This timestamp corresponds to "timestamp" field of RTP packet.
    //!  Just like seqnum, it starts from random value and may overflow.
    stream_timestamp_t stream_timestamp;

    //! Packet duration.
    //! @remarks
    //!  Duration is measured in the same units as timestamp.
    //!  Duration is not stored directly in RTP header. It is calculated
    //!  from packet size.
    stream_timestamp_t duration;

    //! Packet capture timestamp ("cts").
    //! @remarks
    //!  Describes capture time of the first sample using local Unix-time clock.
    //!  This clock belongs to local system, no matter if we're on sender or receiver.
    //!  On sender, capture timestamp is assigned to the system time of sender when
    //!  the first sample in the packet was captured.
    //!  On receiver, capture timestamp is assigned an estimation of the same
    //!  value, converted to receiver system clock, i.e. the system time of receiver
    //!  when the first sample in the packet was captured on sender.
    //!  This field does not directly correspond to anything inside RTP packet.
    //!  Instead, receiver deduces this value based on "timestamp" field from RTP
    //!  packet, current NTP time, and mapping of NTP timestamps to RTP timestamps
    //!  retrieved via RTCP.
    core::nanoseconds_t capture_timestamp;

    //! Packet marker bit ("m").
    //! @remarks
    //!  Marker bit meaning depends on packet type.
    bool marker;

    //! Packet payload type ("pt").
    unsigned int payload_type;

    //! Packet header.
    core::Slice<uint8_t> header;

    //! Packet payload.
    //! @remarks
    //!  Doesn't include RTP headers and padding.
    core::Slice<uint8_t> payload;

    //! Packet padding.
    //! @remarks
    //!  Not included in header and payload, but affects overall packet size.
    core::Slice<uint8_t> padding;

    //! Construct zero RTP packet.
    RTP();

    //! Determine packet order.
    int compare(const RTP&) const;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_RTP_H_
