/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_core/panic.h"

#include "roc_packet/packet_router.h"

namespace roc {
namespace packet {

void PacketRouter::add_route(IPacketConstWriter& writer, int options) {
    if (routes_.size() == routes_.max_size()) {
        roc_panic("packet router: can't add more than %lu routes",
                  (unsigned long)routes_.max_size());
    }

    Route r;
    r.options = options;
    r.writer = &writer;

    routes_.append(r);
}

bool PacketRouter::may_route(const IPacketConstPtr& packet) const {
    return find_route_(packet);
}

bool PacketRouter::may_autodetect_route(const IPacketConstPtr& packet) const {
    return detect_route_(packet) != NULL;
}

void PacketRouter::write(const IPacketConstPtr& packet) {
    if (!packet) {
        roc_panic("packet router: attempting to write null packet");
    }

    if (!packet->order()) {
        roc_panic("packet router: attempting to write packet w/o ordering interface");
    }

    if (Route* route = find_route_(packet)) {
        if ((packet->options() & route->options) == route->options) {
            route->packet = packet;
            route->writer->write(packet);
        } else {
            roc_log(LogDebug,
                    "packet router: packet type mistamatch for route, dropping packet");
        }
        return;
    }

    if (Route* route = detect_route_(packet)) {
        roc_log(LogInfo, "packet router: auto-detected route for new packet: route=%u",
                (unsigned)(route - &routes_[0]));
        route->packet = packet;
        route->writer->write(packet);
        return;
    }

    roc_log(LogDebug, "packet router: no route for packet found, dropping packet");
}

const PacketRouter::Route*
PacketRouter::find_route_(const IPacketConstPtr& packet) const {
    for (size_t n = 0; n < routes_.size(); n++) {
        if (!routes_[n].packet) {
            continue;
        }

        if (routes_[n].packet->order()->is_same_flow(*packet)) {
            return &routes_[n];
        }
    }

    return NULL;
}

const PacketRouter::Route*
PacketRouter::detect_route_(const IPacketConstPtr& packet) const {
    for (size_t n = 0; n < routes_.size(); n++) {
        if (routes_[n].packet) {
            continue;
        }

        if ((packet->options() & routes_[n].options) == routes_[n].options) {
            return &routes_[n];
        }
    }

    return NULL;
}

PacketRouter::Route* PacketRouter::find_route_(const IPacketConstPtr& packet) {
    return const_cast<Route*>(
        static_cast<const PacketRouter*>(this)->find_route_(packet));
}

PacketRouter::Route* PacketRouter::detect_route_(const IPacketConstPtr& packet) {
    return const_cast<Route*>(
        static_cast<const PacketRouter*>(this)->detect_route_(packet));
}

} // namespace packet
} // namespace roc
