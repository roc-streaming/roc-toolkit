/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/udp_receiver_port.h
//! @brief UDP receiver.

#ifndef ROC_NETIO_UDP_RECEIVER_PORT_H_
#define ROC_NETIO_UDP_RECEIVER_PORT_H_

#include <uv.h>

#include "roc_address/socket_addr.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace netio {

//! UDP receiver parameters.
struct UdpReceiverConfig {
    //! Receiver will bind to this address.
    //! If IP is zero, INADDR_ANY is used, i.e. the socket is bound to all network
    //! interfaces. If port is zero, a random free port is selected.
    address::SocketAddr bind_address;

    //! If not empty, receiver will join multicast group on the interface
    //! with given address. May be "0.0.0.0" or "[::]" to join on all interfaces.
    char multicast_interface[64];

    UdpReceiverConfig() {
        multicast_interface[0] = '\0';
    }
};

//! UDP receiver.
class UdpReceiverPort : public BasicPort {
public:
    //! Initialize.
    UdpReceiverPort(const UdpReceiverConfig& config,
                    packet::IWriter& writer,
                    ICloseHandler& close_handler,
                    uv_loop_t& event_loop,
                    packet::PacketPool& packet_pool,
                    core::BufferPool<uint8_t>& buffer_pool,
                    core::IAllocator& allocator);

    //! Destroy.
    ~UdpReceiverPort();

    //! Get bind address.
    virtual const address::SocketAddr& address() const;

    //! Open receiver.
    virtual bool open();

    //! Asynchronously close receiver.
    virtual bool async_close();

private:
    static void close_cb_(uv_handle_t* handle);
    static void alloc_cb_(uv_handle_t* handle, size_t size, uv_buf_t* buf);
    static void recv_cb_(uv_udp_t* handle,
                         ssize_t nread,
                         const uv_buf_t* buf,
                         const sockaddr* addr,
                         unsigned flags);

    bool join_multicast_group_();
    void leave_multicast_group_();

    UdpReceiverConfig config_;
    packet::IWriter& writer_;

    ICloseHandler& close_handler_;

    uv_loop_t& loop_;

    uv_udp_t handle_;
    bool handle_initialized_;

    bool multicast_group_joined_;
    bool recv_started_;
    bool closed_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& buffer_pool_;

    unsigned packet_counter_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_UDP_RECEIVER_PORT_H_
