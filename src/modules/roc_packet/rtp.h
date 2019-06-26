/*
 * Copyright (c) 2017 Roc authors
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
#include "roc_packet/units.h"

namespace roc {
namespace packet {

//! RTP packet.
struct RTP {
    //! Packet source ID identifying packet stream.
    //! @remarks
    //!  Sequence numbers and timestamp are numbered independently inside
    //!  different packet streams.
    source_t source;

    //! Packet sequence number in packet stream.
    //! @remarks
    //!  Packets are numbered sequentaly in every stream, starting from some
    //!  random value. May overflow.
    seqnum_t seqnum;

    //! Packet timestamp.
    //! @remarks
    //!  Timestamp units and exact meaning depends on packet type. For example,
    //!  it may be used to define the number of the first sample in packet, or
    //!  the time when the packet were generated.
    timestamp_t timestamp;

    //! Packet duration.
    //! @remarks
    //!  Duration is measured in the same units as timestamp.
    timestamp_t duration;

    //! Packet marker bit.
    //! @remarks
    //!  Marker bit meaning depends on packet type.
    bool marker;

    //! Packet payload type.
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
