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

#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_packet/address.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace netio {

//! UDP receiver.
class UDPReceiverPort : public BasicPort {
public:
    //! Initialize.
    UDPReceiverPort(ICloseHandler& close_handler,
                    const packet::Address&,
                    uv_loop_t& event_loop,
                    packet::IWriter& writer,
                    packet::PacketPool& packet_pool,
                    core::BufferPool<uint8_t>& buffer_pool,
                    core::IAllocator& allocator);

    //! Destroy.
    ~UDPReceiverPort();

    //! Get bind address.
    virtual const packet::Address& address() const;

    //! Open receiver.
    virtual bool open();

    //! Asynchronously close receiver.
    virtual void async_close();

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

    ICloseHandler& close_handler_;

    uv_loop_t& loop_;

    uv_udp_t handle_;
    bool handle_initialized_;

    bool multicast_group_joined_;
    bool recv_started_;
    bool closed_;

    packet::Address address_;
    packet::IWriter& writer_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& buffer_pool_;

    unsigned packet_counter_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_UDP_RECEIVER_PORT_H_
