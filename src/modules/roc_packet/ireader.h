/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/ireader.h
//! @brief Packet reader interface.

#ifndef ROC_PACKET_IREADER_H_
#define ROC_PACKET_IREADER_H_

#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Packet reader interface.
class IReader {
public:
    virtual ~IReader();

    //! Read packet.
    //! @returns
    //!  next available packet or NULL if there are no packets.
    virtual PacketPtr read() = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IREADER_H_
