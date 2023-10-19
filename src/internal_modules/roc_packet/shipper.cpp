/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/shipper.h"

namespace roc {
namespace packet {

Shipper::Shipper(const address::SocketAddr& dest_address,
                 IComposer& composer,
                 IWriter& writer)
    : dest_address_(dest_address)
    , composer_(composer)
    , writer_(writer) {
}

status::StatusCode Shipper::write(const PacketPtr& packet) {
    if (dest_address_.has_host_port()) {
        packet->add_flags(Packet::FlagUDP);
        packet->udp()->dst_addr = dest_address_;
    }

    if (!packet->has_flags(packet::Packet::FlagPrepared)) {
        roc_panic("shipper: unexpected packet: should be prepared");
    }

    if (!packet->has_flags(packet::Packet::FlagComposed)) {
        if (!composer_.compose(*packet)) {
            roc_panic("shipper: can't compose packet");
        }
        packet->add_flags(Packet::FlagComposed);
    }

    return writer_.write(packet);
}

} // namespace packet
} // namespace roc
