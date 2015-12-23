/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/packet_sender.h
//! @brief Packet sender.

#ifndef ROC_PACKET_PACKET_SENDER_H_
#define ROC_PACKET_PACKET_SENDER_H_

#include "roc_core/noncopyable.h"

#include "roc_packet/ipacket.h"
#include "roc_packet/ipacket_writer.h"

#include "roc_datagram/idatagram.h"
#include "roc_datagram/idatagram_writer.h"
#include "roc_datagram/idatagram_composer.h"

namespace roc {
namespace packet {

//! Packet sender.
//! @remarks
//!  Constructs datagrams from packets and sends them to output writer.
class PacketSender : public IPacketWriter, public core::NonCopyable<> {
public:
    //! Constructor.
    //!
    //! @b Parameters
    //!  - @p writer specifies output writer for constructed datagrams.
    //!  - @p composer us used to construct output datagrams.
    PacketSender(datagram::IDatagramWriter& writer,
                 datagram::IDatagramComposer& composer);

    //! Set sender address for constructed datagrams.
    void set_sender(const datagram::Address&);

    //! Set receiver address for constructed datagrams.
    void set_receiver(const datagram::Address&);

    //! Add packet.
    //! @remarks
    //!  Constructs datagram from packet and sends it to output writer.
    virtual void write(const IPacketPtr&);

private:
    datagram::IDatagramWriter& writer_;
    datagram::IDatagramComposer& composer_;

    datagram::Address sender_;
    datagram::Address receiver_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_PACKET_SENDER_H_
