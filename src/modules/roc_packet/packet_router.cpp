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

#include "roc_packet/packet_router.h"

namespace roc {
namespace packet {

void PacketRouter::add_route(PacketType type, IPacketConstWriter& writer) {
    if (type == NULL) {
        roc_panic("packet router: packet type is null");
    }

    for (size_t n = 0; n < routes_.size(); n++) {
        if (routes_[n].type == type) {
            roc_panic(
                "packet router: can't add more than one route for single packet type");
        }
    }

    if (routes_.size() == routes_.max_size()) {
        roc_panic("packet router: can't add more than %lu routes",
                  (unsigned long)routes_.max_size());
    }

    Route r;
    r.type = type;
    r.writer = &writer;

    routes_.append(r);
}

void PacketRouter::write(const IPacketConstPtr& packet) {
    const PacketType type = packet->type();

    for (size_t n = 0; n < routes_.size(); n++) {
        if (routes_[n].type == type) {
            routes_[n].writer->write(packet);
            return;
        }
    }

    roc_log(LOG_TRACE, "packet router: no route for packet found, dropping packet");
}

} // namespace packet
} // namespace roc
