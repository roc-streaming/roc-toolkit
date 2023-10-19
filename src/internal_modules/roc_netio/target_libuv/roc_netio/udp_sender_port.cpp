/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/udp_sender_port.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/panic.h"
#include "roc_netio/socket_ops.h"

namespace roc {
namespace netio {

namespace {

const core::nanoseconds_t PacketLogInterval = 20 * core::Second;

} // namespace

UdpSenderPort::UdpSenderPort(const UdpSenderConfig& config,
                             uv_loop_t& event_loop,
                             core::IArena& arena)
    : BasicPort(arena)
    , config_(config)
    , close_handler_(NULL)
    , close_handler_arg_(NULL)
    , loop_(event_loop)
    , write_sem_initialized_(false)
    , handle_initialized_(false)
    , pending_packets_(0)
    , sent_packets_(0)
    , sent_packets_blk_(0)
    , stopped_(true)
    , closed_(false)
    , fd_()
    , rate_limiter_(PacketLogInterval) {
    BasicPort::update_descriptor();
}

UdpSenderPort::~UdpSenderPort() {
    if (handle_initialized_ || write_sem_initialized_) {
        roc_panic("udp sender: %s: sender was not fully closed before calling destructor",
                  descriptor());
    }

    if (pending_packets_) {
        roc_panic("udp sender: %s: packets weren't fully sent before calling destructor",
                  descriptor());
    }
}

const address::SocketAddr& UdpSenderPort::bind_address() const {
    return config_.bind_address;
}

bool UdpSenderPort::open() {
    if (int err = uv_async_init(&loop_, &write_sem_, write_sem_cb_)) {
        roc_log(LogError, "udp sender: %s: uv_async_init(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    write_sem_.data = this;
    write_sem_initialized_ = true;

    if (int err = uv_udp_init(&loop_, &handle_)) {
        roc_log(LogError, "udp sender: %s: uv_udp_init(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    handle_.data = this;
    handle_initialized_ = true;

    unsigned flags = 0;
    if (config_.reuseaddr && config_.bind_address.port() > 0) {
        flags |= UV_UDP_REUSEADDR;
    }

    int bind_err = UV_EINVAL;
    if (address_.family() == address::Family_IPv6) {
        bind_err =
            uv_udp_bind(&handle_, config_.bind_address.saddr(), flags | UV_UDP_IPV6ONLY);
    }
    if (bind_err == UV_EINVAL || bind_err == UV_ENOTSUP) {
        bind_err = uv_udp_bind(&handle_, config_.bind_address.saddr(), flags);
    }
    if (bind_err != 0) {
        roc_log(LogError, "udp sender: %s: uv_udp_bind(): [%s] %s", descriptor(),
                uv_err_name(bind_err), uv_strerror(bind_err));
        return false;
    }

    if (int err = uv_udp_set_broadcast(&handle_, 1)) {
        roc_log(LogError, "udp sender: %s: uv_udp_set_broadcast(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    int addrlen = (int)config_.bind_address.slen();
    if (int err = uv_udp_getsockname(&handle_, config_.bind_address.saddr(), &addrlen)) {
        roc_log(LogError, "udp sender: %s: uv_udp_getsockname(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    if (addrlen != (int)config_.bind_address.slen()) {
        roc_log(
            LogError,
            "udp sender: %s: uv_udp_getsockname(): unexpected len: got=%lu expected=%lu",
            descriptor(), (unsigned long)addrlen,
            (unsigned long)config_.bind_address.slen());
        return false;
    }

    const int fd_err = uv_fileno((uv_handle_t*)&handle_, &fd_);
    if (fd_err != 0) {
        roc_panic("udp sender: %s: uv_fileno(): [%s] %s", descriptor(),
                  uv_err_name(fd_err), uv_strerror(fd_err));
    }

    stopped_ = false;
    update_descriptor();

    roc_log(LogDebug, "udp sender: %s: opened port", descriptor());

    return true;
}

AsyncOperationStatus UdpSenderPort::async_close(ICloseHandler& handler,
                                                void* handler_arg) {
    if (close_handler_) {
        roc_panic("udp sender: %s: can't call async_close() twice", descriptor());
    }

    close_handler_ = &handler;
    close_handler_arg_ = handler_arg;

    stopped_ = true;

    if (fully_closed_()) {
        return AsyncOp_Completed;
    }

    if (pending_packets_ == 0) {
        start_closing_();
    }

    return AsyncOp_Started;
}

status::StatusCode UdpSenderPort::write(const packet::PacketPtr& pp) {
    if (!pp) {
        roc_panic("udp sender: %s: unexpected null packet", descriptor());
    }

    if (!pp->udp()) {
        roc_panic("udp sender: %s: unexpected non-udp packet", descriptor());
    }

    if (!pp->data()) {
        roc_panic("udp sender: %s: unexpected packet w/o data", descriptor());
    }

    if (stopped_) {
        roc_panic("udp sender: %s: attempt to use stopped sender", descriptor());
    }

    write_(pp);

    report_stats_();

    return status::StatusOK;
}

void UdpSenderPort::write_(const packet::PacketPtr& pp) {
    const bool had_pending = (++pending_packets_ > 1);

    if (!had_pending) {
        if (try_nonblocking_send_(pp)) {
            --pending_packets_;
            return;
        }
    }

    queue_.push_back(*pp);

    if (int err = uv_async_send(&write_sem_)) {
        roc_panic("udp sender: %s: uv_async_send(): [%s] %s", descriptor(),
                  uv_err_name(err), uv_strerror(err));
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

    roc_log(LogDebug, "udp sender: %s: closed port", self.descriptor());

    roc_panic_if_not(self.close_handler_);

    self.closed_ = true;
    self.close_handler_->handle_close_completed(self, self.close_handler_arg_);
}

void UdpSenderPort::write_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);

    UdpSenderPort& self = *(UdpSenderPort*)handle->data;

    // Using try_pop_front_exclusive() makes this method lock-free and wait-free.
    // try_pop_front_exclusive() may return NULL if the queue is not empty, but
    // push_back() is currently in progress. In this case we can exit the loop
    // before processing all packets, but write() always calls uv_async_send()
    // after push_back(), so we'll wake up soon and process the rest packets.
    while (packet::PacketPtr pp = self.queue_.try_pop_front_exclusive()) {
        packet::UDP& udp = *pp->udp();

        const int packet_num = ++self.sent_packets_;
        ++self.sent_packets_blk_;

        roc_log(LogTrace, "udp sender: %s: sending packet: num=%d src=%s dst=%s sz=%ld",
                self.descriptor(), packet_num,
                address::socket_addr_to_str(self.config_.bind_address).c_str(),
                address::socket_addr_to_str(udp.dst_addr).c_str(),
                (long)pp->data().size());

        uv_buf_t buf;
        buf.base = (char*)pp->data().data();
        buf.len = pp->data().size();

        udp.request.data = &self;

        if (int err = uv_udp_send(&udp.request, &self.handle_, &buf, 1,
                                  udp.dst_addr.saddr(), send_cb_)) {
            roc_log(LogError, "udp sender: %s: uv_udp_send(): [%s] %s", self.descriptor(),
                    uv_err_name(err), uv_strerror(err));
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

    // decrement reference counter incremented in write_sem_cb_()
    pp->decref();

    if (status < 0) {
        roc_log(LogError,
                "udp sender: %s:"
                " can't send packet: src=%s dst=%s sz=%ld: [%s] %s",
                self.descriptor(),
                address::socket_addr_to_str(self.config_.bind_address).c_str(),
                address::socket_addr_to_str(pp->udp()->dst_addr).c_str(),
                (long)pp->data().size(), uv_err_name(status), uv_strerror(status));
    }

    const int pending_packets = --self.pending_packets_;

    if (pending_packets == 0 && self.stopped_) {
        self.start_closing_();
    }
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
        roc_log(LogDebug, "udp sender: %s: initiating asynchronous close", descriptor());

        uv_close((uv_handle_t*)&handle_, close_cb_);
    }

    if (write_sem_initialized_ && !uv_is_closing((uv_handle_t*)&write_sem_)) {
        uv_close((uv_handle_t*)&write_sem_, close_cb_);
    }
}

bool UdpSenderPort::try_nonblocking_send_(const packet::PacketPtr& pp) {
    if (!config_.non_blocking_enabled) {
        return false;
    }

    const packet::UDP& udp = *pp->udp();
    const bool success =
        socket_try_send_to(fd_, pp->data().data(), pp->data().size(), udp.dst_addr);

    if (success) {
        const int packet_num = ++sent_packets_;
        roc_log(LogTrace,
                "udp sender: %s: sent packet non-blocking: num=%d src=%s dst=%s sz=%ld",
                descriptor(), packet_num,
                address::socket_addr_to_str(config_.bind_address).c_str(),
                address::socket_addr_to_str(udp.dst_addr).c_str(),
                (long)pp->data().size());
    }

    return success;
}

void UdpSenderPort::report_stats_() {
    if (!rate_limiter_.allow()) {
        return;
    }

    const int sent_packets = sent_packets_;
    const int sent_packets_nb = (sent_packets - sent_packets_blk_);

    const double nb_ratio =
        sent_packets_nb != 0 ? (double)sent_packets_ / sent_packets_nb : 0.;

    roc_log(LogDebug, "udp sender: %s: total=%u nb=%u nb_ratio=%.5f", descriptor(),
            sent_packets, sent_packets_nb, nb_ratio);
}

void UdpSenderPort::format_descriptor(core::StringBuilder& b) {
    b.append_str("<udpsend");

    b.append_str(" 0x");
    b.append_uint((unsigned long)this, 16);

    b.append_str(" bind=");
    b.append_str(address::socket_addr_to_str(config_.bind_address).c_str());

    b.append_str(">");
}

} // namespace netio
} // namespace roc
