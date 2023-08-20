/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/tcp_server_port.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace netio {

TcpServerPort::TcpServerPort(const TcpServerConfig& config,
                             IConnAcceptor& conn_acceptor,
                             uv_loop_t& loop,
                             core::IArena& arena)
    : BasicPort(arena)
    , config_(config)
    , conn_acceptor_(conn_acceptor)
    , close_handler_(NULL)
    , close_handler_arg_(NULL)
    , loop_(loop)
    , socket_(SocketInvalid)
    , poll_handle_initialized_(false)
    , poll_handle_started_(false)
    , want_close_(false)
    , closed_(false) {
    BasicPort::update_descriptor();
}

TcpServerPort::~TcpServerPort() {
    if (open_conns_.size() != 0) {
        roc_panic("tcp server: %s: server has %d open connection(s) in desructor",
                  descriptor(), (int)open_conns_.size());
    }

    if (closing_conns_.size() != 0) {
        roc_panic("tcp server: %s: server has %d closing connection(s) in desructor",
                  descriptor(), (int)closing_conns_.size());
    }

    if (poll_handle_initialized_ || socket_ != SocketInvalid) {
        roc_panic("tcp server: %s: server was not fully closed before calling destructor",
                  descriptor());
    }
}

const address::SocketAddr& TcpServerPort::bind_address() const {
    return config_.bind_address;
}

bool TcpServerPort::open() {
    if (!socket_create(config_.bind_address.family(), SocketType_Tcp, socket_)) {
        roc_log(LogError, "tcp server: %s: socket_create() failed", descriptor());
        return false;
    }

    if (!socket_setup(socket_, config_.socket_options)) {
        roc_log(LogError, "tcp server: %s: socket_setup() failed", descriptor());
        return false;
    }

    if (!socket_bind(socket_, config_.bind_address)) {
        roc_log(LogError, "tcp server: %s: socket_bind() failed", descriptor());
        return false;
    }

    if (!socket_listen(socket_, config_.backlog_limit)) {
        roc_log(LogError, "tcp server: %s: socket_listen() failed", descriptor());
        return false;
    }

    poll_handle_.data = this;

    if (int err = uv_poll_init_socket(&loop_, &poll_handle_, socket_)) {
        roc_log(LogError, "tcp server: %s: uv_poll_init(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    poll_handle_initialized_ = true;

    if (int err = uv_poll_start(&poll_handle_, UV_READABLE | UV_WRITABLE, poll_cb_)) {
        roc_log(LogError, "tcp server: %s: uv_poll_start(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    poll_handle_started_ = true;

    update_descriptor();

    roc_log(LogDebug, "tcp server: %s: opened port", descriptor());

    return true;
}

AsyncOperationStatus TcpServerPort::async_close(ICloseHandler& handler,
                                                void* handler_arg) {
    if (open_conns_.size() != 0) {
        roc_panic("tcp server: %s: "
                  "can't close tcp server port before terminating all connections",
                  descriptor());
    }

    if (close_handler_) {
        roc_panic("tcp server: %s: can't call async_close() twice", descriptor());
    }

    close_handler_ = &handler;
    close_handler_arg_ = handler_arg;

    if (closed_) {
        return AsyncOp_Completed;
    }

    want_close_ = true;

    if (num_connections_() != 0) {
        return AsyncOp_Started;
    }

    return async_close_server_();
}

void TcpServerPort::poll_cb_(uv_poll_t* handle, int status, int events) {
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TcpServerPort& self = *(TcpServerPort*)handle->data;

    if (status < 0) {
        roc_log(LogError, "tcp server: %s: poll failed: [%s] %s", self.descriptor(),
                uv_err_name(status), uv_strerror(status));
        return;
    }

    if ((events & UV_READABLE) == 0) {
        return;
    }

    roc_log(LogDebug, "tcp server: %s: trying to accept incoming connection",
            self.descriptor());

    core::SharedPtr<TcpConnectionPort> conn =
        new (self.arena()) TcpConnectionPort(TcpConn_Server, self.loop_, self.arena());
    if (!conn) {
        roc_log(LogError, "tcp server: %s: can't allocate connection", self.descriptor());
        return;
    }

    if (!conn->open()) {
        roc_log(LogError, "tcp server: %s: can't open connection", self.descriptor());

        self.async_close_connection_(conn);
        return;
    }

    if (!conn->accept(self.config_, self.config_.bind_address, self.socket_)) {
        roc_log(LogError, "tcp server: %s: can't accept connection", self.descriptor());

        self.async_terminate_connection_(conn);
        return;
    }

    roc_log(LogDebug, "tcp server: %s: adding connection: %s", self.descriptor(),
            conn->descriptor());

    IConnHandler* conn_handler = self.conn_acceptor_.add_connection(*conn);
    if (!conn_handler) {
        roc_log(LogError, "tcp server: %s: can't obtain connection handler",
                self.descriptor());

        self.async_terminate_connection_(conn);
        return;
    }

    // release_usage will be called in handle_terminate_completed()
    conn_handler->incref();

    self.open_conns_.push_back(*conn);

    conn->attach_terminate_handler(self, conn_handler);
    conn->attach_connection_handler(*conn_handler);
}

void TcpServerPort::close_cb_(uv_handle_t* handle) {
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TcpServerPort& self = *(TcpServerPort*)handle->data;

    if (self.closed_) {
        return;
    }

    roc_log(LogDebug, "tcp server: %s: closed port", self.descriptor());

    self.poll_handle_initialized_ = false;

    self.finish_closing_server_();

    roc_panic_if(!self.close_handler_);
    self.closed_ = true;
    self.close_handler_->handle_close_completed(self, self.close_handler_arg_);
}

void TcpServerPort::handle_terminate_completed(IConn& conn, void* arg) {
    core::SharedPtr<TcpConnectionPort> tcp_conn(static_cast<TcpConnectionPort*>(&conn));

    roc_log(LogDebug, "tcp server: %s: asynchronous terminate completed: %s",
            descriptor(), tcp_conn->descriptor());

    async_close_connection_(tcp_conn);

    IConnHandler* conn_handler = (IConnHandler*)arg;
    if (conn_handler) {
        roc_log(LogDebug, "tcp server: %s: removing connection: %s", descriptor(),
                tcp_conn->descriptor());

        conn_handler->decref();
        conn_acceptor_.remove_connection(*conn_handler);
    }
}

void TcpServerPort::handle_close_completed(BasicPort& port, void*) {
    core::SharedPtr<TcpConnectionPort> tcp_conn(static_cast<TcpConnectionPort*>(&port));

    if (!closing_conns_.contains(*tcp_conn)) {
        roc_panic("tcp server: %s: connection is not in closing list: %s", descriptor(),
                  tcp_conn->descriptor());
    }

    roc_log(LogDebug, "tcp server: %s: asynchronous close completed: %s", descriptor(),
            tcp_conn->descriptor());

    finish_closing_connection_(tcp_conn);

    if (want_close_ && num_connections_() == 0) {
        async_close_server_();
    }
}

AsyncOperationStatus TcpServerPort::async_close_server_() {
    if (closed_) {
        return AsyncOp_Completed;
    }

    if (!poll_handle_initialized_) {
        finish_closing_server_();
        closed_ = true;

        return AsyncOp_Completed;
    }

    roc_log(LogDebug, "tcp server: %s: initiating asynchronous close", descriptor());

    if (poll_handle_initialized_ && !uv_is_closing((uv_handle_t*)&poll_handle_)) {
        uv_close((uv_handle_t*)&poll_handle_, close_cb_);
    }

    return AsyncOp_Started;
}

void TcpServerPort::finish_closing_server_() {
    if (socket_ != SocketInvalid) {
        if (!socket_close(socket_)) {
            roc_log(LogError, "tcp server: %s: failed to close socket", descriptor());
        }
        socket_ = SocketInvalid;
    }
}

size_t TcpServerPort::num_connections_() const {
    return open_conns_.size() + closing_conns_.size();
}

void TcpServerPort::async_terminate_connection_(
    const core::SharedPtr<TcpConnectionPort>& conn) {
    if (closing_conns_.contains(*conn)) {
        roc_panic("tcp server: %s: connection is already in closing list: %s",
                  descriptor(), conn->descriptor());
    }

    if (open_conns_.contains(*conn)) {
        open_conns_.remove(*conn);
    }

    closing_conns_.push_back(*conn);

    conn->attach_terminate_handler(*this, NULL);
    conn->async_terminate(Term_Failure);
}

void TcpServerPort::async_close_connection_(
    const core::SharedPtr<TcpConnectionPort>& conn) {
    if (open_conns_.contains(*conn)) {
        open_conns_.remove(*conn);
    }

    const AsyncOperationStatus status = conn->async_close(*this, NULL);

    if (status == AsyncOp_Started) {
        if (!closing_conns_.contains(*conn)) {
            closing_conns_.push_back(*conn);
        }
    }
}

void TcpServerPort::finish_closing_connection_(
    const core::SharedPtr<TcpConnectionPort>& conn) {
    if (!closing_conns_.contains(*conn)) {
        roc_panic("tcp server: %s: connection is not in closing list: %s", descriptor(),
                  conn->descriptor());
    }

    closing_conns_.remove(*conn);
}

void TcpServerPort::format_descriptor(core::StringBuilder& b) {
    b.append_str("<tcpserv");

    b.append_str(" 0x");
    b.append_uint((unsigned long)this, 16);

    b.append_str(" bind=");
    b.append_str(address::socket_addr_to_str(config_.bind_address).c_str());

    b.append_str(">");
}

} // namespace netio
} // namespace roc
