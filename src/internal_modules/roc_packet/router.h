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

namespace roc {
namespace packet {

//! Route packets to writers.
class Router : public IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    Router(core::IArena& arena);

    //! Add route.
    //! @remarks
    //!  Packets that has given @p flags set will be routed to @p writer.
    ROC_ATTR_NODISCARD bool add_route(IWriter& writer, unsigned flags);

    //! Write next packet.
    //! @remarks
    //!  Route @p packet to a writer or drop it if no routes found.
    virtual void write(const PacketPtr& packet);

private:
    struct Route {
        IWriter* writer;
        unsigned flags;
        stream_source_t source;
        bool has_source;
    };

    core::Array<Route, 2> routes_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_ROUTER_H_
