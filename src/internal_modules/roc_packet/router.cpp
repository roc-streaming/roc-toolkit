/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/router.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_packet/packet_flags_to_str.h"
#include "roc_status/status_code.h"

namespace roc {
namespace packet {

Router::Router(core::IArena& arena)
    : routes_(arena) {
}

status::StatusCode Router::init_status() const {
    return status::StatusOK;
}

status::StatusCode Router::add_route(IWriter& writer, const unsigned flags) {
    Route r;
    r.writer = &writer;
    r.flags = flags;

    if (!routes_.push_back(r)) {
        roc_log(LogError, "router: can't allocate route");
        return status::StatusNoMem;
    }

    return status::StatusOK;
}

bool Router::has_source_id(const unsigned flags) {
    if (Route* route = find_route_(flags)) {
        return route->has_source;
    }

    return false;
}

stream_source_t Router::get_source_id(const unsigned flags) {
    if (Route* route = find_route_(flags)) {
        return route->has_source ? route->source : 0;
    }

    return 0;
}

status::StatusCode Router::write(const PacketPtr& packet) {
    if (!packet) {
        roc_panic("router: unexpected null packet");
    }

    if (Route* route = find_route_(packet->flags())) {
        if (allow_route_(*route, *packet)) {
            if (packet->has_flags(Packet::FlagUDP)
                && packet->udp()->queue_timestamp == 0) {
                packet->udp()->queue_timestamp = core::timestamp(core::ClockUnix);
            }

            return route->writer->write(packet);
        }
    }

    roc_log(LogDebug, "router: can't route packet, dropping: source=%lu flags=%s",
            (unsigned long)packet->source_id(),
            packet_flags_to_str(packet->flags()).c_str());

    return status::StatusNoRoute;
}

Router::Route* Router::find_route_(unsigned flags) {
    for (size_t n = 0; n < routes_.size(); n++) {
        Route& route = routes_[n];

        if ((route.flags & flags) == route.flags) {
            return &route;
        }
    }

    return NULL;
}

bool Router::allow_route_(Route& route, const Packet& packet) {
    if (packet.has_source_id()) {
        if (route.has_source) {
            if (route.source != packet.source_id()) {
                // Route is started and has different source id.
                // No match.
                return false;
            }
        } else {
            if (route.is_started) {
                // Route is started and has no source id, but packet has one.
                // No match.
                return false;
            }

            // Route is not started, start and remember source id.
            route.source = packet.source_id();
            route.has_source = true;
            route.is_started = true;

            roc_log(LogNote,
                    "router: detected new stream:"
                    " source_id=%lu route_flags=%s packet_flags=%s",
                    (unsigned long)route.source, packet_flags_to_str(route.flags).c_str(),
                    packet_flags_to_str(packet.flags()).c_str());
        }
    } else {
        if (route.has_source) {
            // Route is started and has source id, but packet doesn't have one.
            // No match.
            return false;
        }

        if (!route.is_started) {
            // Route is not started, start and remember that there is no source id.
            route.has_source = false;
            route.is_started = true;

            roc_log(LogNote,
                    "router: detected new stream:"
                    " source_id=none route_flags=%s packet_flags=%s",
                    packet_flags_to_str(route.flags).c_str(),
                    packet_flags_to_str(packet.flags()).c_str());
        }
    }

    // Match!
    return true;
}

} // namespace packet
} // namespace roc
