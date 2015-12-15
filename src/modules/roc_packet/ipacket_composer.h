/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/ipacket_composer.h
//! @brief Packet composer interface.

#ifndef ROC_PACKET_IPACKET_COMPOSER_H_
#define ROC_PACKET_IPACKET_COMPOSER_H_

#include "roc_packet/ipacket.h"

namespace roc {
namespace packet {

//! Packet composer interface.
class IPacketComposer {
public:
    virtual ~IPacketComposer();

    //! Create new packet.
    //!
    //! @returns
    //!  new packet or NULL if packet of given @p type can't be created.
    //!
    //! @remarks
    //!  All fields of returned packet are set to default values (usually
    //!  zeros). Caller can downcast packet to derived interface and
    //!  configure packet's fields.
    //!
    //! @b Example usage
    //!  @code
    //!   if (IPacketPtr packet = composer.compose(IAudioPacket::Type)) {
    //!       roc_panic_if_not(packet->type() == IAudioPacket::Type);
    //!
    //!       IAudioPacketPtr audio = static_cast<IAudioPacket*>(packet.get());
    //!       audio->set_timestamp(123);
    //!   }
    //!  @endcode
    virtual IPacketPtr compose(PacketType type) = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IPACKET_COMPOSER_H_
