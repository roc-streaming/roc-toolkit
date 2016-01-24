/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/ipacket.h
//! @brief Packet interface.

#ifndef ROC_PACKET_IPACKET_H_
#define ROC_PACKET_IPACKET_H_

#include "roc_core/byte_buffer.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"
#include "roc_core/shared_ptr.h"

#include "roc_packet/iheader_ordering.h"
#include "roc_packet/iheader_rtp.h"
#include "roc_packet/iheader_fecframe.h"
#include "roc_packet/ipayload_audio.h"

namespace roc {
namespace packet {

//! Packet interface.
class IPacket : public core::RefCnt, public core::ListNode {
public:
    //! Packet options.
    enum {
        HasOrder = (1 << 0), //!< Packet contains ordering/routing information.
        HasRTP = (1 << 1),   //!< Packet contains RTP header.
        HasFEC = (1 << 2),   //!< Packet contains FECFRAME header.
        HasAudio = (1 << 3)  //!< Packet contains audio payload.
    };

    virtual ~IPacket();

    //! Get packet options.
    virtual int options() const = 0;

    //! Get abstract header for ordering/routing (if there is one).
    virtual const IHeaderOrdering* order() const = 0;

    //! Get RTP header (if there is one).
    virtual const IHeaderRTP* rtp() const = 0;

    //! Get RTP header (if there is one).
    virtual IHeaderRTP* rtp() = 0;

    //! Get FECFRAME header (if there is one).
    virtual const IHeaderFECFrame* fec() const = 0;

    //! Get FECFRAME header (if there is one).
    virtual IHeaderFECFrame* fec() = 0;

    //! Get audio payload (if there is one).
    virtual const IPayloadAudio* audio() const = 0;

    //! Get audio payload (if there is one).
    virtual IPayloadAudio* audio() = 0;

    //! Get packet data buffer (containing header and payload).
    virtual core::IByteBufferConstSlice raw_data() const = 0;

    //! Get packet payload.
    virtual core::IByteBufferConstSlice payload() const = 0;

    //! Set payload data and size.
    virtual void set_payload(const uint8_t* data, size_t size) = 0;

    //! Print packet to stderr.
    virtual void print(bool print_payload = false) const;
};

//! Packet smart pointer.
typedef core::SharedPtr<IPacket> IPacketPtr;

//! Const packet smart pointer.
typedef core::SharedPtr<IPacket const> IPacketConstPtr;

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IPACKET_H_
