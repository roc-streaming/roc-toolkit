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

void PacketSender::write(const IPacketPtr& packet) {
    if (!packet) {
        roc_panic("packet sender: packet is null");
    }

    datagram::IDatagramPtr dgm = composer_.compose();
    if (!dgm) {
        roc_log(LogError, "packet sender: can't allocate datagram, dropping packet");
        return;
    }

    const Port* port = find_port_(packet->options());
    if (!port) {
        roc_panic("packet sender: no port found for packet: options=%x",
                  (unsigned)packet->options());
    }

    dgm->set_buffer(packet->raw_data());
    dgm->set_sender(port->send_addr);
    dgm->set_receiver(port->recv_addr);

    writer_.write(dgm);
}

void PacketSender::add_port(const datagram::Address& source,
                            const datagram::Address& destination,
                            int options) {
    Port port;
    port.send_addr = source;
    port.recv_addr = destination;
    port.options = options;

    ports_.append(port);
}

const PacketSender::Port* PacketSender::find_port_(int options) const {
    for (size_t n = 0; n < ports_.size(); n++) {
        if ((options & ports_[n].options) == ports_[n].options) {
            return &ports_[n];
        }
    }
    return NULL;
}

} // namespace packet
} // namespace roc
