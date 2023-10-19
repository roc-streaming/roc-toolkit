/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/udp_sender_port.h
//! @brief UDP sender.

#ifndef ROC_NETIO_UDP_SENDER_PORT_H_
#define ROC_NETIO_UDP_SENDER_PORT_H_

#include <uv.h>

#include "roc_address/socket_addr.h"
#include "roc_core/atomic.h"
#include "roc_core/iarena.h"
#include "roc_core/mpsc_queue.h"
#include "roc_core/rate_limiter.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_packet/iwriter.h"

namespace roc {
namespace netio {

//! UDP sender parameters.
struct UdpSenderConfig {
    //! Sender will bind to this address.
    //! If IP is zero, INADDR_ANY is used, i.e. the socket is bound to all network
    //! interfaces. If port is zero, a random free port is selected.
    address::SocketAddr bind_address;

    //! If set, enable SO_REUSEADDR when binding socket to non-ephemeral port.
    //! If not set, SO_REUSEADDR is not enabled.
    bool reuseaddr;

    //! If true, allow non-blocking writes directly in write() method.
    //! If non-blocking write can't be performed, sender falls back to
    //! regular asynchronous write.
    bool non_blocking_enabled;

    UdpSenderConfig()
        : reuseaddr(false)
        , non_blocking_enabled(true) {
    }

    //! Check two configs for equality.
    bool operator==(const UdpSenderConfig& other) const {
        return bind_address == other.bind_address
            && non_blocking_enabled == other.non_blocking_enabled;
    }
};

//! UDP sender.
class UdpSenderPort : public BasicPort, public packet::IWriter {
public:
    //! Initialize.
    UdpSenderPort(const UdpSenderConfig& config,
                  uv_loop_t& event_loop,
                  core::IArena& arena);

    //! Destroy.
    ~UdpSenderPort();

    //! Get bind address.
    const address::SocketAddr& bind_address() const;

    //! Open sender.
    virtual bool open();

    //! Asynchronously close sender.
    virtual AsyncOperationStatus async_close(ICloseHandler& handler, void* handler_arg);

    //! Write packet.
    //! @remarks
    //!  May be called from any thread.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr&);

protected:
    //! Format descriptor.
    virtual void format_descriptor(core::StringBuilder& b);

private:
    static void close_cb_(uv_handle_t* handle);
    static void write_sem_cb_(uv_async_t* handle);
    static void send_cb_(uv_udp_send_t* req, int status);

    void write_(const packet::PacketPtr&);

    bool fully_closed_() const;
    void start_closing_();

    bool try_nonblocking_send_(const packet::PacketPtr& pp);
    void report_stats_();

    UdpSenderConfig config_;

    ICloseHandler* close_handler_;
    void* close_handler_arg_;

    uv_loop_t& loop_;

    uv_async_t write_sem_;
    bool write_sem_initialized_;

    uv_udp_t handle_;
    bool handle_initialized_;

    address::SocketAddr address_;

    core::MpscQueue<packet::Packet> queue_;

    core::Atomic<int> pending_packets_;
    core::Atomic<int> sent_packets_;
    core::Atomic<int> sent_packets_blk_;

    bool stopped_;
    bool closed_;

    uv_os_fd_t fd_;

    core::RateLimiter rate_limiter_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_UDP_SENDER_PORT_H_
