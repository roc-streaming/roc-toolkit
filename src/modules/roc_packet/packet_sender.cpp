/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/log.h"

#include "roc_packet/packet_sender.h"

namespace roc {
namespace packet {

PacketSender::PacketSender(datagram::IDatagramWriter& writer,
                           datagram::IDatagramComposer& composer)
    : writer_(writer)
    , composer_(composer) {
}

void PacketSender::set_sender(const datagram::Address& address) {
    sender_ = address;
}

void PacketSender::set_receiver(const datagram::Address& address) {
    receiver_ = address;
}

void PacketSender::write(const IPacketPtr& packet) {
    if (!packet) {
        roc_panic("packet sender: packet is null");
    }

    datagram::IDatagramPtr dgm = composer_.compose();
    if (!dgm) {
        roc_log(LogError, "packet sender: can't allocate datagram, dropping packet");
        return;
    }

    dgm->set_buffer(packet->raw_data());
    dgm->set_sender(sender_);
    dgm->set_receiver(receiver_);

    writer_.write(dgm);
}

} // namespace packet
} // namespace roc
