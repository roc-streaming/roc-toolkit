/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/packet_router.h
//! @brief Packet router.

#ifndef ROC_PACKET_PACKET_ROUTER_H_
#define ROC_PACKET_PACKET_ROUTER_H_

#include "roc_config/config.h"

#include "roc_core/noncopyable.h"
#include "roc_core/array.h"

#include "roc_packet/ipacket.h"
#include "roc_packet/ipacket_writer.h"

namespace roc {
namespace packet {

//! Packet router.
//! @remarks
//!  Routes packets to multiple writers by source ID. For every route,
//!  source ID may be auto-detected from first packing of matching type.
class PacketRouter : public IPacketConstWriter, public core::NonCopyable<> {
public:
    //! Add route for packets with given options.
    void add_route(IPacketConstWriter& writer, int options);

    //! Check if there is a route already associated with packet's source ID.
    //! @returns
    //!  true if there is a route for given source ID.
    bool may_route(const IPacketConstPtr&) const;

    //! Check if there is a route that may be associated packet's source ID.
    //! @returns
    //!  true if there is a route without source ID and with matching
    //!  packet type.
    bool may_autodetect_route(const IPacketConstPtr&) const;

    //! Write packet.
    //! @remarks
    //!  If route found for packet's source id, packet is sent to corresponding
    //!  writer. Otherwise, packet is dropped.
    virtual void write(const IPacketConstPtr&);

private:
    static const size_t MaxRoutes = ROC_CONFIG_MAX_SESSION_QUEUES;

    struct Route {
        int options;
        IPacketConstPtr packet;
        IPacketConstWriter* writer;

        Route()
            : options(0)
            , packet()
            , writer(NULL) {
        }
    };

    const Route* find_route_(const IPacketConstPtr&) const;
    Route* find_route_(const IPacketConstPtr&);

    const Route* detect_route_(const IPacketConstPtr&) const;
    Route* detect_route_(const IPacketConstPtr&);

    core::Array<Route, MaxRoutes> routes_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_PACKET_ROUTER_H_
