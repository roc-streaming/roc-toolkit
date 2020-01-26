/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/udp_sender_port.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/helpers.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace netio {

UdpSenderPort::UdpSenderPort(ICloseHandler& close_handler,
                             const address::SocketAddr& address,
                             uv_loop_t& event_loop,
                             core::IAllocator& allocator)
    : BasicPort(allocator)
    , close_handler_(close_handler)
    , loop_(event_loop)
    , write_sem_initialized_(false)
    , handle_initialized_(false)
    , address_(address)
    , pending_(0)
    , stopped_(true)
    , closed_(false)
    , packet_counter_(0) {
}

UdpSenderPort::~UdpSenderPort() {
    if (handle_initialized_ || write_sem_initialized_) {
        roc_panic("udp sender: sender was not fully closed before calling destructor");
    }
}

const address::SocketAddr& UdpSenderPort::address() const {
    return address_;
}

bool UdpSenderPort::open() {
    if (int err = uv_async_init(&loop_, &write_sem_, write_sem_cb_)) {
        roc_log(LogError, "udp sender: uv_async_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    write_sem_.data = this;
    write_sem_initialized_ = true;

    if (int err = uv_udp_init(&loop_, &handle_)) {
        roc_log(LogError, "udp sender: uv_udp_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    handle_.data = this;
    handle_initialized_ = true;

    unsigned flags = 0;
    if (address_.multicast() && address_.port() > 0) {
        flags |= UV_UDP_REUSEADDR;
    }

    int bind_err = UV_EINVAL;
    if (address_.family() == address::Family_IPv6) {
        bind_err = uv_udp_bind(&handle_, address_.saddr(), flags | UV_UDP_IPV6ONLY);
    }
    if (bind_err == UV_EINVAL || bind_err == UV_ENOTSUP) {
        bind_err = uv_udp_bind(&handle_, address_.saddr(), flags);
    }
    if (bind_err != 0) {
        roc_log(LogError, "udp sender: uv_udp_bind(): [%s] %s", uv_err_name(bind_err),
                uv_strerror(bind_err));
        return false;
    }

    if (address_.broadcast()) {
        roc_log(LogDebug, "udp sender: setting broadcast flag for port %s",
                address::socket_addr_to_str(address_).c_str());

        if (int err = uv_udp_set_broadcast(&handle_, 1)) {
            roc_log(LogError, "udp sender: uv_udp_set_broadcast(): [%s] %s",
                    uv_err_name(err), uv_strerror(err));
            return false;
        }
    }

    int addrlen = (int)address_.slen();
    if (int err = uv_udp_getsockname(&handle_, address_.saddr(), &addrlen)) {
        roc_log(LogError, "udp sender: uv_udp_getsockname(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    if (addrlen != (int)address_.slen()) {
        roc_log(LogError,
                "udp sender: uv_udp_getsockname(): unexpected len: got=%lu expected=%lu",
                (unsigned long)addrlen, (unsigned long)address_.slen());
        return false;
    }

    roc_log(LogInfo, "udp sender: opened port %s",
            address::socket_addr_to_str(address_).c_str());

    stopped_ = false;

    return true;
}

bool UdpSenderPort::async_close() {
    core::Mutex::Lock lock(mutex_);

    stopped_ = true;

    if (fully_closed_()) {
        return false;
    }

    if (pending_ == 0) {
        start_closing_();
    }

    return true;
}

void UdpSenderPort::write(const packet::PacketPtr& pp) {
    if (!pp) {
        roc_panic("udp sender: unexpected null packet");
    }

    if (!pp->udp()) {
        roc_panic("udp sender: unexpected non-udp packet");
    }

    if (!pp->data()) {
        roc_panic("udp sender: unexpected packet w/o data");
    }

    {
        core::Mutex::Lock lock(mutex_);

        if (stopped_) {
            return;
        }

        list_.push_back(*pp);
        ++pending_;
    }

    if (int err = uv_async_send(&write_sem_)) {
        roc_panic("udp sender: uv_async_send(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }
}

void UdpSenderPort::close_cb_(uv_handle_t* handle) {
    roc_panic_if_not(handle);

    UdpSenderPort& self = *(UdpSenderPort*)handle->data;

    if (handle == (uv_handle_t*)&self.handle_) {
        self.handle_initialized_ = false;
    } else {
        self.write_sem_initialized_ = false;
    }

    if (self.handle_initialized_ || self.write_sem_initialized_) {
        return;
    }

    roc_log(LogInfo, "udp sender: closed port %s",
            address::socket_addr_to_str(self.address_).c_str());

    self.closed_ = true;
    self.close_handler_.handle_closed(self);
}

void UdpSenderPort::write_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);

    UdpSenderPort& self = *(UdpSenderPort*)handle->data;

    while (packet::PacketPtr pp = self.read_()) {
        packet::UDP& udp = *pp->udp();

        self.packet_counter_++;

        roc_log(LogTrace, "udp sender: sending packet: num=%u src=%s dst=%s sz=%ld",
                self.packet_counter_, address::socket_addr_to_str(self.address_).c_str(),
                address::socket_addr_to_str(udp.dst_addr).c_str(),
                (long)pp->data().size());

        uv_buf_t buf;
        buf.base = (char*)pp->data().data();
        buf.len = pp->data().size();

        udp.request.data = &self;

        if (int err = uv_udp_send(&udp.request, &self.handle_, &buf, 1,
                                  udp.dst_addr.saddr(), send_cb_)) {
            roc_log(LogError, "udp sender: uv_udp_send(): [%s] %s", uv_err_name(err),
                    uv_strerror(err));
            continue;
        }

        // will be decremented in send_cb_()
        pp->incref();
    }
}

void UdpSenderPort::send_cb_(uv_udp_send_t* req, int status) {
    roc_panic_if_not(req);

    UdpSenderPort& self = *(UdpSenderPort*)req->data;

    packet::PacketPtr pp =
        packet::Packet::container_of(ROC_CONTAINER_OF(req, packet::UDP, request));

    // one reference for incref() called from write_sem_cb_()
    // one reference for the shared pointer above
    roc_panic_if(pp->getref() < 2);

    // decrement reference counter incremented in async_cb_()
    pp->decref();

    if (status < 0) {
        roc_log(LogError,
                "udp sender:"
                " can't send packet: src=%s dst=%s sz=%ld: [%s] %s",
                address::socket_addr_to_str(self.address_).c_str(),
                address::socket_addr_to_str(pp->udp()->dst_addr).c_str(),
                (long)pp->data().size(), uv_err_name(status), uv_strerror(status));
    }

    core::Mutex::Lock lock(self.mutex_);

    --self.pending_;

    if (self.stopped_ && self.pending_ == 0) {
        self.start_closing_();
    }
}

packet::PacketPtr UdpSenderPort::read_() {
    core::Mutex::Lock lock(mutex_);

    packet::PacketPtr pp = list_.front();
    if (pp) {
        list_.remove(*pp);
    }

    return pp;
}

bool UdpSenderPort::fully_closed_() const {
    if (!handle_initialized_ && !write_sem_initialized_) {
        return true;
    }

    if (closed_) {
        return true;
    }

    return false;
}

void UdpSenderPort::start_closing_() {
    if (fully_closed_()) {
        return;
    }

    if (handle_initialized_ && !uv_is_closing((uv_handle_t*)&handle_)) {
        roc_log(LogInfo, "udp sender: closing port %s",
                address::socket_addr_to_str(address_).c_str());

        uv_close((uv_handle_t*)&handle_, close_cb_);
    }

    if (write_sem_initialized_ && !uv_is_closing((uv_handle_t*)&write_sem_)) {
        uv_close((uv_handle_t*)&write_sem_, close_cb_);
    }
}

} // namespace netio
} // namespace roc
