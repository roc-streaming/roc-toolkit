/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/iheader_ordering.h
//! @brief Packet ordering/routing header interface.

#ifndef ROC_PACKET_IHEADER_ORDERING_H_
#define ROC_PACKET_IHEADER_ORDERING_H_

namespace roc {
namespace packet {

class IPacket;

//! Packet ordering/routing header interface.
//! @remarks
//!  Provides protocol-independent interface for packet routing and ordering.
//!  May be implemented, e.g. on top of RTP or FECFRAME headers.
class IHeaderOrdering {
public:
    virtual ~IHeaderOrdering();

    //! Check if two packets are from the same flow.
    virtual bool is_same_flow(const IPacket& other) const = 0;

    //! Check if this packet is before another packet in flow.
    virtual bool is_before(const IPacket& other) const = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IHEADER_ORDERING_H_
