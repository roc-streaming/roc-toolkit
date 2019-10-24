/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/tcp_server_port.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"

namespace roc {
namespace netio {

TCPServerPort::TCPServerPort(const address::SocketAddr& address,
                             uv_loop_t& loop,
                             ICloseHandler& close_handler,
                             IConnAcceptor& conn_acceptor,
                             core::IAllocator& allocator)
    : BasicPort(allocator)
    , close_handler_(close_handler)
    , conn_acceptor_(conn_acceptor)
    , loop_(loop)
    , handle_initialized_(false)
    , closed_(false)
    , stopped_(true)
    , address_(address) {
}

TCPServerPort::~TCPServerPort() {
    roc_panic_if(open_ports_.size());
    roc_panic_if(closing_ports_.size());
    roc_panic_if(handle_initialized_);
}

const address::SocketAddr& TCPServerPort::address() const {
    return address_;
}

bool TCPServerPort::open() {
    if (int err = uv_tcp_init(&loop_, &handle_)) {
        roc_log(LogError, "tcp server: uv_tcp_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }
    handle_.data = this;
    handle_initialized_ = true;

    unsigned flags = 0;

    int bind_err = UV_EINVAL;
    if (address_.version() == 6) {
        bind_err = uv_tcp_bind(&handle_, address_.saddr(), flags | UV_TCP_IPV6ONLY);
    }
    if (bind_err == UV_EINVAL || bind_err == UV_ENOTSUP) {
        bind_err = uv_tcp_bind(&handle_, address_.saddr(), flags);
    }
    if (bind_err != 0) {
        roc_log(LogError, "tcp server: uv_tcp_bind(): [%s] %s", uv_err_name(bind_err),
                uv_strerror(bind_err));
        return false;
    }

    int addrlen = (int)address_.slen();
    if (int err = uv_tcp_getsockname(&handle_, address_.saddr(), &addrlen)) {
        roc_log(LogError, "tcp server: uv_tcp_getsockname(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    if (addrlen != (int)address_.slen()) {
        roc_log(LogError,
                "tcp server: uv_tcp_getsockname(): unexpected len: got=%lu expected=%lu",
                (unsigned long)addrlen, (unsigned long)address_.slen());
        return false;
    }

    if (int err = uv_listen((uv_stream_t*)&handle_, Backlog, listen_cb_)) {
        roc_log(LogError, "tcp server: uv_listen(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    roc_log(LogInfo, "tcp server: opened port %s",
            address::socket_addr_to_str(address_).c_str());

    return true;
}

void TCPServerPort::async_close() {
    stopped_ = true;

    if (num_ports_()) {
        async_close_ports_();
    } else {
        close_();
    }
}

void TCPServerPort::close_cb_(uv_handle_t* handle) {
    roc_panic_if_not(handle);

    TCPServerPort& self = *(TCPServerPort*)handle->data;

    self.closed_ = true;
    self.handle_initialized_ = false;

    roc_log(LogInfo, "tcp server: closed port %s",
            address::socket_addr_to_str(self.address_).c_str());

    self.close_handler_.handle_closed(self);
}

void TCPServerPort::listen_cb_(uv_stream_t* stream, int status) {
    if (status < 0) {
        roc_log(LogError, "tcp server: failed to connect: [%s] %s", uv_err_name(status),
                uv_strerror(status));

        return;
    }

    roc_panic_if_not(stream);
    roc_panic_if_not(stream->data);

    TCPServerPort& self = *(TCPServerPort*)stream->data;

    core::SharedPtr<TCPConn> cp = new (self.allocator_)
        TCPConn(self.address_, "server", self.loop_, self, self.allocator_);
    if (!cp) {
        roc_log(LogError, "tcp server: can't allocate connection");

        return;
    }

    if (!cp->open()) {
        roc_log(LogError, "tcp server: can't open connection");

        self.closing_ports_.push_back(*cp);
        cp->async_close();

        return;
    }

    IConnNotifier* conn_notifier = self.conn_acceptor_.accept(*cp);
    if (!conn_notifier) {
        roc_log(LogError, "tcp server: can't get connection notifier");

        self.closing_ports_.push_back(*cp);
        cp->async_close();

        return;
    }

    if (!cp->accept(stream, *conn_notifier)) {
        roc_log(LogError, "tcp server: can't accept connection");

        self.closing_ports_.push_back(*cp);
        cp->async_close();

        return;
    }

    self.open_ports_.push_back(*cp);

    roc_log(LogInfo, "tcp server: accepted: src=%s dst=%s",
            address::socket_addr_to_str(cp->address()).c_str(),
            address::socket_addr_to_str(cp->destination_address()).c_str());
}

void TCPServerPort::handle_closed(BasicPort& port) {
    roc_panic_if_not(remove_closing_port_(port));

    if (stopped_ && num_ports_() == 0) {
        close_();
    }
}

size_t TCPServerPort::num_ports_() const {
    return open_ports_.size() + closing_ports_.size();
}

void TCPServerPort::close_() {
    if (closed_) {
        return; // handle_closed() was already called
    }

    if (!handle_initialized_) {
        closed_ = true;
        close_handler_.handle_closed(*this);

        return;
    }

    roc_log(LogInfo, "tcp server: closing port %s",
            address::socket_addr_to_str(address_).c_str());

    if (!uv_is_closing((uv_handle_t*)&handle_)) {
        uv_close((uv_handle_t*)&handle_, close_cb_);
    }
}

void TCPServerPort::async_close_ports_() {
    while (core::SharedPtr<BasicPort> port = open_ports_.front()) {
        open_ports_.remove(*port);
        closing_ports_.push_back(*port);

        port->async_close();
    }
}

bool TCPServerPort::remove_closing_port_(BasicPort& port) {
    for (core::SharedPtr<BasicPort> pp = closing_ports_.front(); pp;
         pp = closing_ports_.nextof(*pp)) {
        if (pp.get() != &port) {
            continue;
        }

        roc_log(LogDebug, "tcp server: remove connection: port %s",
                address::socket_addr_to_str(port.address()).c_str());

        closing_ports_.remove(*pp);

        return true;
    }

    return false;
}

} // namespace netio
} // namespace roc
