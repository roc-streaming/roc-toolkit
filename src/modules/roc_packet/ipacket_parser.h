/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/ipacket_parser.h
//! @brief Packet parser interface.

#ifndef ROC_PACKET_IPACKET_PARSER_H_
#define ROC_PACKET_IPACKET_PARSER_H_

#include "roc_packet/ipacket.h"

namespace roc {
namespace packet {

//! Packet parser interface.
class IPacketParser {
public:
    virtual ~IPacketParser();

    //! Parse packet from buffer.
    //! @returns
    //!  new packet or NULL if packet can not be parsed.
    //! @remarks
    //!  Returned packet will keep reference to buffer.
    virtual IPacketConstPtr parse(const core::IByteBufferConstSlice& buffer) = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IPACKET_PARSER_H_
