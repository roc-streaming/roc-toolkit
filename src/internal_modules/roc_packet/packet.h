/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/packet.h
//! @brief Packet.

#ifndef ROC_PACKET_PACKET_H_
#define ROC_PACKET_PACKET_H_

#include "roc_core/list_node.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/mpsc_queue_node.h"
#include "roc_core/pool.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"
#include "roc_packet/fec.h"
#include "roc_packet/print_packet.h"
#include "roc_packet/rtcp.h"
#include "roc_packet/rtp.h"
#include "roc_packet/udp.h"

namespace roc {
namespace packet {

class Packet;

//! Packet smart pointer.
typedef core::SharedPtr<Packet> PacketPtr;

//! Packet.
class Packet : public core::RefCounted<Packet, core::PoolAllocation>,
               public core::ListNode,
               public core::MpscQueueNode {
public:
    //! Constructor.
    explicit Packet(core::IPool& packet_pool);

    //! Packet flags.
    enum {
        FlagUDP = (1 << 0),      //!< Packet contains UDP header.
        FlagRTP = (1 << 1),      //!< Packet contains RTP header.
        FlagFEC = (1 << 2),      //!< Packet contains FEC header.
        FlagRTCP = (1 << 3),     //!< Packet contains RTCP compound packet.
        FlagAudio = (1 << 4),    //!< Packet contains audio samples.
        FlagRepair = (1 << 5),   //!< Packet contains repair FEC symbols.
        FlagControl = (1 << 6),  //!< Packet contains control message.
        FlagPrepared = (1 << 7), //!< Packet was prepared for composing.
        FlagComposed = (1 << 8), //!< Packet was composed.
        FlagRestored = (1 << 9)  //!< Packet was restored using FEC decoder.
    };

    //! Add flags.
    void add_flags(unsigned flags);

    //! Check specific flag.
    bool has_flags(unsigned flags) const;

    //! Get flags.
    unsigned flags() const;

    //! UDP packet.
    const UDP* udp() const;

    //! UDP packet.
    UDP* udp();

    //! RTP packet.
    const RTP* rtp() const;

    //! RTP packet.
    RTP* rtp();

    //! FEC packet.
    const FEC* fec() const;

    //! FEC packet.
    FEC* fec();

    //! RTCP packet.
    const RTCP* rtcp() const;

    //! RTCP packet.
    RTCP* rtcp();

    //! Get packet data.
    const core::Slice<uint8_t>& data() const;

    //! Set packet data.
    void set_data(const core::Slice<uint8_t>& data);

    //! Return packet stream identifier.
    //! @remarks
    //!  The returning value depends on packet type. For some packet types, may
    //!  be always zero.
    stream_source_t source() const;

    //! Get the timestamp of the first sample in packet.
    //! @remarks
    //!  Timestamp units depend on packet type. For some packet types, may
    //!  be always zero.
    stream_timestamp_t begin() const;

    //! Get the timestamp of the last sample in packet plus one.
    //! @remarks
    //!  Timestamp units depend on packet type. For some packet types, may
    //!  be always zero.
    stream_timestamp_t end() const;

    //! Determine packet order.
    //! @returns
    //!  * -1 if this packet precedes @p other packet
    //!  *  0 if this packet has the same position as @p other packet
    //!  * +1 if this packet succeeds @p other packet
    int compare(const Packet& other) const;

    //! Print packet to stderr.
    void print(int flags) const {
        packet::print_packet(*this, flags);
    }

    //! Get pointer to packet from a pointer to its UDP part.
    static Packet* container_of(UDP* udp) {
        return ROC_CONTAINER_OF(udp, Packet, udp_);
    }

private:
    unsigned flags_;

    UDP udp_;
    RTP rtp_;
    FEC fec_;
    RTCP rtcp_;

    core::Slice<uint8_t> data_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_PACKET_H_
