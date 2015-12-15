/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/ipacket_writer.h
//! @brief Packet writer interface.

#ifndef ROC_PACKET_IPACKET_WRITER_H_
#define ROC_PACKET_IPACKET_WRITER_H_

#include "roc_packet/ipacket.h"

namespace roc {
namespace packet {

//! Packet writer interface.
class IPacketWriter {
public:
    virtual ~IPacketWriter();

    //! Add packet.
    virtual void write(const IPacketPtr&) = 0;
};

//! Packet const writer interface.
class IPacketConstWriter : public IPacketWriter {
public:
    virtual ~IPacketConstWriter();

    //! Add packet.
    virtual void write(const IPacketConstPtr&) = 0;

    //! Add packet.
    virtual void write(const IPacketPtr& packet) {
        write(IPacketConstPtr(packet));
    }
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IPACKET_WRITER_H_
