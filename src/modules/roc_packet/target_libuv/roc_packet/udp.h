/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/target_libuv/roc_packet/udp.h
//! @brief UDP packet.

#ifndef ROC_PACKET_UDP_H_
#define ROC_PACKET_UDP_H_

#include <uv.h>

#include "roc_address/socket_addr.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace packet {

//! UDP packet.
struct UDP {
    //! Source address.
    address::SocketAddr src_addr;

    //! Destination address.
    address::SocketAddr dst_addr;

    //! Sender request state.
    uv_udp_send_t request;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_UDP_H_
