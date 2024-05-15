/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/udp_port.h
//! @brief UDP port.

#ifndef ROC_NETIO_UDP_PORT_H_
#define ROC_NETIO_UDP_PORT_H_

#include <uv.h>

#include "roc_address/socket_addr.h"
#include "roc_core/iarena.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/mpsc_queue.h"
#include "roc_core/rate_limiter.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_factory.h"

namespace roc {
namespace netio {

//! UDP port parameters.
struct UdpConfig {
    //! Port will bind to this address.
    //! If IP is zero, INADDR_ANY is used, i.e. the socket is bound to all network
    //! interfaces. If port is zero, a random free port is selected.
    address::SocketAddr bind_address;

    //! If not empty, port will join multicast group on the interface
    //! with given address. May be "0.0.0.0" or "[::]" to join on all interfaces.
    //! Used only if receiving is started.
    char multicast_interface[64];

    //! If set, enable SO_REUSEADDR when binding socket to non-ephemeral port.
    //! If not set, SO_REUSEADDR is enabled only for multicast sockets when
    //! binding to non-ephemeral port.
    bool enable_reuseaddr;

    //! If true, allow non-blocking writes directly in write() method.
    //! If non-blocking write can't be performed, port falls back to
    //! regular asynchronous write.
    //! Used only if sending is started.
    bool enable_non_blocking;

    UdpConfig()
        : enable_reuseaddr(false)
        , enable_non_blocking(true) {
        multicast_interface[0] = '\0';
    }

    //! Check two configs for equality.
    bool operator==(const UdpConfig& other) const {
        return bind_address == other.bind_address
            && strcmp(multicast_interface, other.multicast_interface) == 0
            && enable_reuseaddr == other.enable_reuseaddr
            && enable_non_blocking == other.enable_non_blocking;
    }
};

//! UDP sender/receiver port.
class UdpPort : public BasicPort, private packet::IWriter {
public:
    //! Initialize.
    UdpPort(const UdpConfig& config,
            uv_loop_t& event_loop,
            packet::PacketFactory& packet_factory,
            core::IArena& arena);

    //! Destroy.
    virtual ~UdpPort();

    //! Get bind address.
    const address::SocketAddr& bind_address() const;

    //! Open receiver.
    virtual bool open();

    //! Asynchronously close receiver.
    virtual AsyncOperationStatus async_close(ICloseHandler& handler, void* handler_arg);

    //! Start receiving packets.
    //! @remarks
    //!  Packets written to returned writer will be enqueued for sending.
    //!  Writer can be used from any thread.
    packet::IWriter* start_send();

    //! Start receiving packets.
    //! @remarks
    //!  Received packets will be written to inbound_writer.
    //!  Writer will be invoked from network thread.
    bool start_recv(packet::IWriter& inbound_writer);

protected:
    //! Format descriptor.
    virtual void format_descriptor(core::StringBuilder& b);

private:
    static void close_cb_(uv_handle_t* handle);

    static void alloc_cb_(uv_handle_t* handle, size_t size, uv_buf_t* buf);
    static void recv_cb_(uv_udp_t* handle,
                         ssize_t nread,
                         const uv_buf_t* buf,
                         const sockaddr* addr,
                         unsigned flags);

    static void write_sem_cb_(uv_async_t* handle);
    static void send_cb_(uv_udp_send_t* req, int status);

    // Implements packet::IWriter::write()
    virtual status::StatusCode write(const packet::PacketPtr& packet);
    void write_(const packet::PacketPtr& packet);
    bool try_nonblocking_write_(const packet::PacketPtr& pp);

    bool fully_closed_() const;
    void start_closing_();

    bool join_multicast_group_();
    void leave_multicast_group_();

    void report_stats_();

    UdpConfig config_;

    ICloseHandler* close_handler_;
    void* close_handler_arg_;

    uv_loop_t& loop_;

    uv_udp_t handle_;
    bool handle_initialized_;

    uv_async_t write_sem_;
    bool write_sem_initialized_;

    bool multicast_group_joined_;
    bool recv_started_;
    bool want_close_;
    bool closed_;

    uv_os_fd_t fd_;

    packet::PacketFactory& packet_factory_;

    packet::IWriter* inbound_writer_;
    core::MpscQueue<packet::Packet> outbound_queue_;

    core::RateLimiter rate_limiter_;

    core::Atomic<int> pending_packets_;
    core::Atomic<int> sent_packets_;
    core::Atomic<int> sent_packets_blk_;
    core::Atomic<int> received_packets_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_UDP_PORT_H_
