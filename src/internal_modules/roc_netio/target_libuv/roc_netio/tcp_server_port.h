/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/tcp_server_port.h
//! @brief TCP server.

#ifndef ROC_NETIO_TCP_SERVER_PORT_H_
#define ROC_NETIO_TCP_SERVER_PORT_H_

#include <uv.h>

#include "roc_address/socket_addr.h"
#include "roc_core/iarena.h"
#include "roc_core/list.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/stddefs.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_netio/iconn_acceptor.h"
#include "roc_netio/iterminate_handler.h"
#include "roc_netio/socket_ops.h"
#include "roc_netio/tcp_connection_port.h"

namespace roc {
namespace netio {

//! TCP server parameters.
struct TcpServerConfig : TcpConnectionConfig {
    //! Server will bind to this address.
    //! If IP is zero, INADDR_ANY is used, i.e. the socket is bound to all network
    //! interfaces. If port is zero, a random free port is selected.
    address::SocketAddr bind_address;

    //! Maximum length to which the queue of pending connections may grow.
    size_t backlog_limit;

    TcpServerConfig()
        : backlog_limit(128) {
    }
};

//! TCP server.
class TcpServerPort : public BasicPort, private ITerminateHandler, private ICloseHandler {
public:
    //! Initialize.
    TcpServerPort(const TcpServerConfig& config,
                  IConnAcceptor& conn_acceptor,
                  uv_loop_t& loop,
                  core::IArena& arena);

    //! Destroy.
    virtual ~TcpServerPort();

    //! Get bind address.
    const address::SocketAddr& bind_address() const;

    //! Open TCP server.
    //!
    //! @remarks
    //!  Should be called from the network loop thread.
    virtual bool open();

    //! Asynchronously close TCP server.
    //!
    //! @remarks
    //!  Should be called from network loop thread.
    virtual AsyncOperationStatus async_close(ICloseHandler& handler, void* handler_arg);

protected:
    //! Format descriptor.
    virtual void format_descriptor(core::StringBuilder& b);

private:
    static void poll_cb_(uv_poll_t* handle, int status, int events);
    static void close_cb_(uv_handle_t* handle);

    virtual void handle_terminate_completed(IConn& conn, void* arg);
    virtual void handle_close_completed(BasicPort& port, void* arg);

    AsyncOperationStatus async_close_server_();
    void finish_closing_server_();

    size_t num_connections_() const;
    void async_close_all_connections_();
    void async_terminate_connection_(const core::SharedPtr<TcpConnectionPort>&);
    void async_close_connection_(const core::SharedPtr<TcpConnectionPort>&);
    void finish_closing_connection_(const core::SharedPtr<TcpConnectionPort>&);

    TcpServerConfig config_;

    IConnAcceptor& conn_acceptor_;

    ICloseHandler* close_handler_;
    void* close_handler_arg_;

    uv_loop_t& loop_;

    SocketHandle socket_;

    uv_poll_t poll_handle_;
    bool poll_handle_initialized_;
    bool poll_handle_started_;

    core::List<TcpConnectionPort> open_conns_;
    core::List<TcpConnectionPort> closing_conns_;

    bool want_close_;
    bool closed_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TCP_SERVER_PORT_H_
