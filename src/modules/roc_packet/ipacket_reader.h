/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/ipacket_reader.h
//! @brief Packet reader interface.

#ifndef ROC_PACKET_IPACKET_READER_H_
#define ROC_PACKET_IPACKET_READER_H_

#include "roc_packet/ipacket.h"

namespace roc {
namespace packet {

//! Packet reader interface.
class IPacketReader {
public:
    virtual ~IPacketReader();

    //! Read next packet.
    //! @returns
    //!  next available packet or NULL if there are no packets.
    //! @remarks
    //!  If non-NULL packet returned, it always has seqnum greater than seqnum
    //!  of previously returned packet.
    //! @note
    //!  Seqnum overflow may occur, use ROC_IS_BEFORE() macro to compare them
    //!  correctly.
    virtual IPacketConstPtr read() = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IPACKET_READER_H_
