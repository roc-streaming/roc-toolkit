/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/udp_receiver_port.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/string_builder.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace netio {

UdpReceiverPort::UdpReceiverPort(const UdpReceiverConfig& config,
                                 packet::IWriter& writer,
                                 uv_loop_t& event_loop,
                                 packet::PacketFactory& packet_factory,
                                 core::BufferFactory<uint8_t>& buffer_factory,
                                 core::IArena& arena)
    : BasicPort(arena)
    , config_(config)
    , writer_(writer)
    , close_handler_(NULL)
    , close_handler_arg_(NULL)
    , loop_(event_loop)
    , handle_initialized_(false)
    , multicast_group_joined_(false)
    , recv_started_(false)
    , closed_(false)
    , packet_factory_(packet_factory)
    , buffer_factory_(buffer_factory)
    , packet_counter_(0) {
    BasicPort::update_descriptor();
}

UdpReceiverPort::~UdpReceiverPort() {
    if (handle_initialized_) {
        roc_panic(
            "udp receiver: %s: receiver was not fully closed before calling destructor",
            descriptor());
    }
}

const address::SocketAddr& UdpReceiverPort::bind_address() const {
    return config_.bind_address;
}

bool UdpReceiverPort::open() {
    if (int err = uv_udp_init(&loop_, &handle_)) {
        roc_log(LogError, "udp receiver: %s: uv_udp_init(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    handle_.data = this;
    handle_initialized_ = true;

    unsigned flags = 0;
    if ((config_.reuseaddr || config_.bind_address.multicast())
        && config_.bind_address.port() > 0) {
        flags |= UV_UDP_REUSEADDR;
    }

    int bind_err = UV_EINVAL;
    if (config_.bind_address.family() == address::Family_IPv6) {
        bind_err =
            uv_udp_bind(&handle_, config_.bind_address.saddr(), flags | UV_UDP_IPV6ONLY);
    }
    if (bind_err == UV_EINVAL || bind_err == UV_ENOTSUP) {
        bind_err = uv_udp_bind(&handle_, config_.bind_address.saddr(), flags);
    }

    if (bind_err != 0) {
        roc_log(LogError, "udp receiver: %s: uv_udp_bind(): [%s] %s", descriptor(),
                uv_err_name(bind_err), uv_strerror(bind_err));
        return false;
    }

    int addrlen = (int)config_.bind_address.slen();
    if (int err = uv_udp_getsockname(&handle_, config_.bind_address.saddr(), &addrlen)) {
        roc_log(LogError, "udp receiver: %s: uv_udp_getsockname(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    if (addrlen != (int)config_.bind_address.slen()) {
        roc_log(LogError,
                "udp receiver: %s:"
                " uv_udp_getsockname(): unexpected len: got=%lu expected=%lu",
                descriptor(), (unsigned long)addrlen,
                (unsigned long)config_.bind_address.slen());
        return false;
    }

    if (config_.multicast_interface[0]) {
        if (!join_multicast_group_()) {
            return false;
        }
    }

    if (int err = uv_udp_recv_start(&handle_, alloc_cb_, recv_cb_)) {
        roc_log(LogError, "udp receiver: %s: uv_udp_recv_start(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    recv_started_ = true;
    update_descriptor();

    roc_log(LogDebug, "udp receiver: %s: opened port", descriptor());

    return true;
}

AsyncOperationStatus UdpReceiverPort::async_close(ICloseHandler& handler,
                                                  void* handler_arg) {
    if (close_handler_) {
        roc_panic("udp receiver: %s: can't call async_close() twice", descriptor());
    }

    close_handler_ = &handler;
    close_handler_arg_ = handler_arg;

    if (!handle_initialized_) {
        return AsyncOp_Completed;
    }

    if (closed_) {
        return AsyncOp_Completed;
    }

    roc_log(LogDebug, "udp receiver: %s: initiating asynchronous close", descriptor());

    if (recv_started_) {
        if (int err = uv_udp_recv_stop(&handle_)) {
            roc_log(LogError, "udp receiver: %s: uv_udp_recv_stop(): [%s] %s",
                    descriptor(), uv_err_name(err), uv_strerror(err));
        }
        recv_started_ = false;
    }

    if (multicast_group_joined_) {
        leave_multicast_group_();
    }

    if (!uv_is_closing((uv_handle_t*)&handle_)) {
        uv_close((uv_handle_t*)&handle_, close_cb_);
    }

    return AsyncOp_Started;
}

void UdpReceiverPort::close_cb_(uv_handle_t* handle) {
    roc_panic_if_not(handle);

    UdpReceiverPort& self = *(UdpReceiverPort*)handle->data;

    self.handle_initialized_ = false;

    roc_log(LogDebug, "udp receiver: %s: closed port", self.descriptor());

    roc_panic_if_not(self.close_handler_);

    self.closed_ = true;
    self.close_handler_->handle_close_completed(self, self.close_handler_arg_);
}

void UdpReceiverPort::alloc_cb_(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
    roc_panic_if_not(handle);
    roc_panic_if_not(buf);

    UdpReceiverPort& self = *(UdpReceiverPort*)handle->data;

    core::SharedPtr<core::Buffer<uint8_t> > bp = self.buffer_factory_.new_buffer();
    if (!bp) {
        roc_log(LogError, "udp receiver: %s: can't allocate buffer", self.descriptor());

        buf->base = NULL;
        buf->len = 0;

        return;
    }

    if (size > bp->size()) {
        size = bp->size();
    }

    bp->incref(); // will be decremented in recv_cb_()

    buf->base = (char*)bp->data();
    buf->len = size;
}

void UdpReceiverPort::recv_cb_(uv_udp_t* handle,
                               ssize_t nread,
                               const uv_buf_t* buf,
                               const sockaddr* sockaddr,
                               unsigned flags) {
    roc_panic_if_not(handle);
    roc_panic_if_not(buf);

    UdpReceiverPort& self = *(UdpReceiverPort*)handle->data;

    address::SocketAddr src_addr;
    if (sockaddr) {
        if (!src_addr.set_host_port_saddr(sockaddr)) {
            roc_log(LogError,
                    "udp receiver: %s:"
                    " can't determine source address: num=%u dst=%s nread=%ld",
                    self.descriptor(), self.packet_counter_,
                    address::socket_addr_to_str(self.config_.bind_address).c_str(),
                    (long)nread);
        }
    }

    core::SharedPtr<core::Buffer<uint8_t> > bp =
        core::Buffer<uint8_t>::container_of(buf->base);

    // one reference for incref() called from alloc_cb_()
    // one reference for the shared pointer above
    roc_panic_if(bp->getref() != 2);

    // decrement reference counter incremented in alloc_cb_()
    bp->decref();

    if (nread < 0) {
        roc_log(
            LogError, "udp receiver: %s: network error: num=%u src=%s dst=%s nread=%ld",
            self.descriptor(), self.packet_counter_,
            address::socket_addr_to_str(src_addr).c_str(),
            address::socket_addr_to_str(self.config_.bind_address).c_str(), (long)nread);
        return;
    }

    if (nread == 0) {
        if (!sockaddr) {
            // no more data for now
        } else {
            roc_log(LogTrace, "udp receiver: %s: empty packet: num=%u src=%s dst=%s",
                    self.descriptor(), self.packet_counter_,
                    address::socket_addr_to_str(src_addr).c_str(),
                    address::socket_addr_to_str(self.config_.bind_address).c_str());
        }
        return;
    }

    if (!sockaddr) {
        roc_panic("udp receiver: %s: unexpected null source address", self.descriptor());
    }

    if (flags & UV_UDP_PARTIAL) {
        roc_log(LogDebug,
                "udp receiver: %s:"
                " ignoring partial read: num=%u src=%s dst=%s nread=%ld",
                self.descriptor(), self.packet_counter_,
                address::socket_addr_to_str(src_addr).c_str(),
                address::socket_addr_to_str(self.config_.bind_address).c_str(),
                (long)nread);
        return;
    }

    self.packet_counter_++;

    roc_log(LogTrace, "udp receiver: %s: received packet: num=%u src=%s dst=%s nread=%ld",
            self.descriptor(), self.packet_counter_,
            address::socket_addr_to_str(src_addr).c_str(),
            address::socket_addr_to_str(self.config_.bind_address).c_str(), (long)nread);

    if ((size_t)nread > bp->size()) {
        roc_panic("udp receiver: %s: unexpected buffer size: got %ld, max %ld",
                  self.descriptor(), (long)nread, (long)bp->size());
    }

    packet::PacketPtr pp = self.packet_factory_.new_packet();
    if (!pp) {
        roc_log(LogError, "udp receiver: %s: can't allocate packet", self.descriptor());
        return;
    }

    pp->add_flags(packet::Packet::FlagUDP);

    pp->udp()->src_addr = src_addr;
    pp->udp()->dst_addr = self.config_.bind_address;

    pp->set_data(core::Slice<uint8_t>(*bp, 0, (size_t)nread));

    const status::StatusCode code = self.writer_.write(pp);
    roc_panic_if(code != status::StatusOK);
}

bool UdpReceiverPort::join_multicast_group_() {
    if (!config_.bind_address.multicast()) {
        roc_log(LogError,
                "udp receiver: %s: can't use multicast group for non-multicast address",
                descriptor());
        return false;
    }

    char host[address::SocketAddr::MaxStrLen];
    if (!config_.bind_address.get_host(host, sizeof(host))) {
        roc_log(LogError, "udp receiver: %s: can't format address host", descriptor());
        return false;
    }

    if (int err = uv_udp_set_membership(&handle_, host, config_.multicast_interface,
                                        UV_JOIN_GROUP)) {
        roc_log(LogError, "udp receiver: %s: uv_udp_set_membership(): [%s] %s",
                descriptor(), uv_err_name(err), uv_strerror(err));
        return false;
    }

    roc_log(LogDebug, "udp receiver: %s: joined multicast group", descriptor());

    return (multicast_group_joined_ = true);
}

void UdpReceiverPort::leave_multicast_group_() {
    multicast_group_joined_ = false;

    char host[address::SocketAddr::MaxStrLen];
    if (!config_.bind_address.get_host(host, sizeof(host))) {
        roc_log(LogError, "udp receiver: %s: can't format address host", descriptor());
        return;
    }

    if (int err = uv_udp_set_membership(&handle_, host, config_.multicast_interface,
                                        UV_LEAVE_GROUP)) {
        roc_log(LogError, "udp receiver: %s: uv_udp_set_membership(): [%s] %s",
                descriptor(), uv_err_name(err), uv_strerror(err));
    }

    roc_log(LogDebug, "udp receiver: %s: left multicast group", descriptor());
}

void UdpReceiverPort::format_descriptor(core::StringBuilder& b) {
    b.append_str("<udprecv");

    b.append_str(" 0x");
    b.append_uint((unsigned long)this, 16);

    b.append_str(" bind=");
    b.append_str(address::socket_addr_to_str(config_.bind_address).c_str());

    b.append_str(">");
}

} // namespace netio
} // namespace roc
