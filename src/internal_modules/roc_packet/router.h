/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/router.h
//! @brief Route packets to writers.

#ifndef ROC_PACKET_ROUTER_H_
#define ROC_PACKET_ROUTER_H_

#include "roc_core/array.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"
#include "roc_packet/units.h"

namespace roc {
namespace packet {

//! Route packets to packet writers.
//!
//! To create a route, user provides packet writer and packet flags.
//! Packets that include specified flags will be routed to given writer.
//!
//! When the very first packet is routed to a writer, router remembers
//! which source id (SSRC) that packet has, or that the packet doesn't
//! have any source id. Then router ensures that only packets with
//! that source id are passed to same writer.
//!
//! The user can query which source id were detected for which routes.
class Router : public IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    Router(core::IArena& arena);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Add route.
    //! @remarks
    //!  Packets that has given @p flags set will be routed to @p writer.
    ROC_ATTR_NODISCARD status::StatusCode add_route(IWriter& writer, unsigned flags);

    //! Check if there is detected source id for given route.
    //! @remarks
    //!  Returns true if there is route for given flags, and packets were
    //!  already written to that route, and those packets have source id.
    bool has_source_id(unsigned flags);

    //! Get detected source id for given route.
    //! @remarks
    //!  If has_source_id() returns true, this method returns source id
    //!  for the route.
    stream_source_t get_source_id(unsigned flags);

    //! Write next packet.
    //! @remarks
    //!  Route @p packet to a writer or drop it if no routes found.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const PacketPtr& packet);

private:
    struct Route {
        IWriter* writer;
        unsigned flags;
        stream_source_t source;
        bool has_source;
        bool is_started;

        Route()
            : writer(NULL)
            , flags(0)
            , source(0)
            , has_source(false)
            , is_started(false) {
        }
    };

    Route* find_route_(unsigned flags);
    bool allow_route_(Route& route, const Packet& packet);

    core::Array<Route, 2> routes_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_ROUTER_H_
