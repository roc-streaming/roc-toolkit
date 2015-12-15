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
class PacketRouter : public IPacketConstWriter, public core::NonCopyable<> {
public:
    //! Add route for packets of given type.
    void add_route(PacketType type, IPacketConstWriter& writer);

    //! Write packet.
    //!
    //! @remarks
    //!  If route found for packet's type, packet is stored in route's writer.
    //!  Otherwise, packet is dropped.
    virtual void write(const IPacketConstPtr&);

private:
    static const size_t MaxRoutes = ROC_CONFIG_MAX_SESSION_QUEUES;

    struct Route {
        PacketType type;
        IPacketConstWriter* writer;

        Route()
            : type(NULL)
            , writer(NULL) {
        }
    };

    core::Array<Route, MaxRoutes> routes_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_PACKET_ROUTER_H_
