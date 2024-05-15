/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/udp_port.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/string_builder.h"
#include "roc_core/time.h"
#include "roc_netio/socket_ops.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace netio {

namespace {

const core::nanoseconds_t PacketLogInterval = 20 * core::Second;

} // namespace

UdpPort::UdpPort(const UdpConfig& config,
                 uv_loop_t& event_loop,
                 packet::PacketFactory& packet_factory,
                 core::IArena& arena)
    : BasicPort(arena)
    , config_(config)
    , close_handler_(NULL)
    , close_handler_arg_(NULL)
    , loop_(event_loop)
    , handle_initialized_(false)
    , write_sem_initialized_(false)
    , multicast_group_joined_(false)
    , recv_started_(false)
    , want_close_(false)
    , closed_(false)
    , fd_()
    , packet_factory_(packet_factory)
    , inbound_writer_(NULL)
    , rate_limiter_(PacketLogInterval) {
    BasicPort::update_descriptor();
}

UdpPort::~UdpPort() {
    if (handle_initialized_) {
        roc_panic("udp port: %s: port was not fully closed before calling destructor",
                  descriptor());
    }

    if (pending_packets_) {
        roc_panic("udp port: %s: packets weren't fully sent before calling destructor",
                  descriptor());
    }
}

const address::SocketAddr& UdpPort::bind_address() const {
    return config_.bind_address;
}

bool UdpPort::open() {
    if (int err = uv_udp_init(&loop_, &handle_)) {
        roc_log(LogError, "udp port: %s: uv_udp_init(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    handle_.data = this;
    handle_initialized_ = true;

    unsigned flags = 0;
    if ((config_.enable_reuseaddr || config_.bind_address.multicast())
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
        roc_log(LogError, "udp port: %s: uv_udp_bind(): [%s] %s", descriptor(),
                uv_err_name(bind_err), uv_strerror(bind_err));
        return false;
    }

    if (int err = uv_udp_set_broadcast(&handle_, 1)) {
        roc_log(LogError, "udp port: %s: uv_udp_set_broadcast(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    int addrlen = (int)config_.bind_address.slen();
    if (int err = uv_udp_getsockname(&handle_, config_.bind_address.saddr(), &addrlen)) {
        roc_log(LogError, "udp port: %s: uv_udp_getsockname(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    if (addrlen != (int)config_.bind_address.slen()) {
        roc_log(LogError,
                "udp port: %s:"
                " uv_udp_getsockname(): unexpected len: got=%lu expected=%lu",
                descriptor(), (unsigned long)addrlen,
                (unsigned long)config_.bind_address.slen());
        return false;
    }

    const int fd_err = uv_fileno((uv_handle_t*)&handle_, &fd_);
    if (fd_err != 0) {
        roc_panic("udp port: %s: uv_fileno(): [%s] %s", descriptor(), uv_err_name(fd_err),
                  uv_strerror(fd_err));
    }

    update_descriptor();

    roc_log(LogDebug, "udp port: %s: opened port", descriptor());

    return true;
}

AsyncOperationStatus UdpPort::async_close(ICloseHandler& handler, void* handler_arg) {
    if (close_handler_) {
        roc_panic("udp port: %s: can't call async_close() twice", descriptor());
    }

    close_handler_ = &handler;
    close_handler_arg_ = handler_arg;

    want_close_ = true;

    if (fully_closed_()) {
        return AsyncOp_Completed;
    }

    if (pending_packets_ == 0) {
        start_closing_();
    }

    return AsyncOp_Started;
}

packet::IWriter* UdpPort::start_send() {
    if (!handle_initialized_) {
        return NULL;
    }

    if (!write_sem_initialized_) {
        if (int err = uv_async_init(&loop_, &write_sem_, write_sem_cb_)) {
            roc_log(LogError, "udp port: %s: uv_async_init(): [%s] %s", descriptor(),
                    uv_err_name(err), uv_strerror(err));
            return NULL;
        }

        write_sem_.data = this;
        write_sem_initialized_ = true;
    }

    return this;
}

bool UdpPort::start_recv(packet::IWriter& inbound_writer) {
    if (!handle_initialized_) {
        return false;
    }

    if (config_.multicast_interface[0] && !multicast_group_joined_) {
        if (!join_multicast_group_()) {
            return false;
        }
    }

    if (!recv_started_) {
        if (int err = uv_udp_recv_start(&handle_, alloc_cb_, recv_cb_)) {
            roc_log(LogError, "udp port: %s: uv_udp_recv_start(): [%s] %s", descriptor(),
                    uv_err_name(err), uv_strerror(err));
            return false;
        }
        recv_started_ = true;
    }

    inbound_writer_ = &inbound_writer;
    return true;
}

void UdpPort::close_cb_(uv_handle_t* handle) {
    roc_panic_if_not(handle);

    UdpPort& self = *(UdpPort*)handle->data;

    if (handle == (uv_handle_t*)&self.handle_) {
        self.handle_initialized_ = false;
    } else {
        self.write_sem_initialized_ = false;
    }

    if (self.handle_initialized_ || self.write_sem_initialized_) {
        return;
    }

    roc_log(LogDebug, "udp port: %s: closed port", self.descriptor());

    roc_panic_if_not(self.close_handler_);

    self.closed_ = true;
    self.close_handler_->handle_close_completed(self, self.close_handler_arg_);
}

void UdpPort::alloc_cb_(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
    roc_panic_if_not(handle);
    roc_panic_if_not(buf);

    UdpPort& self = *(UdpPort*)handle->data;

    core::BufferPtr bp = self.packet_factory_.new_packet_buffer();
    if (!bp) {
        roc_log(LogError, "udp port: %s: can't allocate buffer", self.descriptor());

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

void UdpPort::recv_cb_(uv_udp_t* handle,
                       ssize_t nread,
                       const uv_buf_t* buf,
                       const sockaddr* sockaddr,
                       unsigned flags) {
    roc_panic_if_not(handle);
    roc_panic_if_not(buf);

    UdpPort& self = *(UdpPort*)handle->data;

    address::SocketAddr src_addr;
    if (sockaddr) {
        if (!src_addr.set_host_port_saddr(sockaddr)) {
            roc_log(LogError,
                    "udp port: %s:"
                    " can't determine source address: num=%d dst=%s nread=%ld",
                    self.descriptor(), (int)self.received_packets_,
                    address::socket_addr_to_str(self.config_.bind_address).c_str(),
                    (long)nread);
        }
    }

    core::BufferPtr bp = core::Buffer::container_of(buf->base);

    // one reference for incref() called from alloc_cb_()
    // one reference for the shared pointer above
    roc_panic_if(bp->getref() != 2);

    // decrement reference counter incremented in alloc_cb_()
    bp->decref();

    if (nread < 0) {
        roc_log(LogError, "udp port: %s: network error: num=%d src=%s dst=%s nread=%ld",
                self.descriptor(), (int)self.received_packets_,
                address::socket_addr_to_str(src_addr).c_str(),
                address::socket_addr_to_str(self.config_.bind_address).c_str(),
                (long)nread);
        return;
    }

    if (nread == 0) {
        if (!sockaddr) {
            // no more data for now
        } else {
            roc_log(LogTrace, "udp port: %s: empty packet: num=%d src=%s dst=%s",
                    self.descriptor(), (int)self.received_packets_,
                    address::socket_addr_to_str(src_addr).c_str(),
                    address::socket_addr_to_str(self.config_.bind_address).c_str());
        }
        return;
    }

    if (!sockaddr) {
        roc_panic("udp port: %s: unexpected null source address", self.descriptor());
    }

    if (flags & UV_UDP_PARTIAL) {
        roc_log(LogDebug,
                "udp port: %s:"
                " ignoring partial read: num=%d src=%s dst=%s nread=%ld",
                self.descriptor(), (int)self.received_packets_,
                address::socket_addr_to_str(src_addr).c_str(),
                address::socket_addr_to_str(self.config_.bind_address).c_str(),
                (long)nread);
        return;
    }

    self.received_packets_++;

    roc_log(LogTrace, "udp port: %s: received packet: num=%d src=%s dst=%s nread=%ld",
            self.descriptor(), (int)self.received_packets_,
            address::socket_addr_to_str(src_addr).c_str(),
            address::socket_addr_to_str(self.config_.bind_address).c_str(), (long)nread);

    if ((size_t)nread > bp->size()) {
        roc_panic("udp port: %s: unexpected buffer size: got %ld, max %ld",
                  self.descriptor(), (long)nread, (long)bp->size());
    }

    packet::PacketPtr pp = self.packet_factory_.new_packet();
    if (!pp) {
        roc_log(LogError, "udp port: %s: can't allocate packet", self.descriptor());
        return;
    }

    pp->add_flags(packet::Packet::FlagUDP);

    pp->udp()->src_addr = src_addr;
    pp->udp()->dst_addr = self.config_.bind_address;
    pp->udp()->receive_timestamp = core::timestamp(core::ClockUnix);

    pp->set_buffer(core::Slice<uint8_t>(*bp, 0, (size_t)nread));

    if (self.inbound_writer_) {
        const status::StatusCode code = self.inbound_writer_->write(pp);
        if (code != status::StatusOK) {
            roc_panic("udp port: %s: can't writer packet: status=%s", self.descriptor(),
                      status::code_to_str(code));
        }
    }
}

void UdpPort::write_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);

    UdpPort& self = *(UdpPort*)handle->data;

    // Using try_pop_front_exclusive() makes this method lock-free and wait-free.
    // try_pop_front_exclusive() may return NULL if the queue is not empty, but
    // push_back() is currently in progress. In this case we can exit the loop
    // before processing all packets, but write() always calls uv_async_send()
    // after push_back(), so we'll wake up soon and process the rest packets.
    while (packet::PacketPtr pp = self.outbound_queue_.try_pop_front_exclusive()) {
        packet::UDP& udp = *pp->udp();

        const int packet_num = ++self.sent_packets_;
        ++self.sent_packets_blk_;

        roc_log(LogTrace, "udp port: %s: sending packet: num=%d src=%s dst=%s sz=%ld",
                self.descriptor(), packet_num,
                address::socket_addr_to_str(self.config_.bind_address).c_str(),
                address::socket_addr_to_str(udp.dst_addr).c_str(),
                (long)pp->buffer().size());

        uv_buf_t buf;
        buf.base = (char*)pp->buffer().data();
        buf.len = pp->buffer().size();

        udp.request.data = &self;

        if (int err = uv_udp_send(&udp.request, &self.handle_, &buf, 1,
                                  udp.dst_addr.saddr(), send_cb_)) {
            roc_log(LogError, "udp port: %s: uv_udp_send(): [%s] %s", self.descriptor(),
                    uv_err_name(err), uv_strerror(err));
            continue;
        }

        // will be decremented in send_cb_()
        pp->incref();
    }
}

void UdpPort::send_cb_(uv_udp_send_t* req, int status) {
    roc_panic_if_not(req);

    UdpPort& self = *(UdpPort*)req->data;

    packet::PacketPtr pp =
        packet::Packet::container_of(ROC_CONTAINER_OF(req, packet::UDP, request));

    // one reference for incref() called from write_sem_cb_()
    // one reference for the shared pointer above
    roc_panic_if(pp->getref() < 2);

    // decrement reference counter incremented in write_sem_cb_()
    pp->decref();

    if (status < 0) {
        roc_log(LogError,
                "udp port: %s:"
                " can't send packet: src=%s dst=%s sz=%ld: [%s] %s",
                self.descriptor(),
                address::socket_addr_to_str(self.config_.bind_address).c_str(),
                address::socket_addr_to_str(pp->udp()->dst_addr).c_str(),
                (long)pp->buffer().size(), uv_err_name(status), uv_strerror(status));
    }

    const int pending_packets = --self.pending_packets_;

    if (pending_packets == 0 && self.want_close_) {
        self.start_closing_();
    }
}

status::StatusCode UdpPort::write(const packet::PacketPtr& pp) {
    if (!pp) {
        roc_panic("udp port: %s: unexpected null packet", descriptor());
    }

    if (!pp->udp()) {
        roc_panic("udp port: %s: unexpected non-udp packet", descriptor());
    }

    if (!pp->buffer()) {
        roc_panic("udp port: %s: unexpected packet w/o buffer", descriptor());
    }

    if (want_close_) {
        roc_panic("udp port: %s: attempt to use closed sender", descriptor());
    }

    write_(pp);

    report_stats_();

    return status::StatusOK;
}

void UdpPort::write_(const packet::PacketPtr& pp) {
    const bool had_pending = (++pending_packets_ > 1);
    if (!had_pending) {
        if (try_nonblocking_write_(pp)) {
            --pending_packets_;
            return;
        }
    }

    outbound_queue_.push_back(*pp);

    if (int err = uv_async_send(&write_sem_)) {
        roc_panic("udp port: %s: uv_async_send(): [%s] %s", descriptor(),
                  uv_err_name(err), uv_strerror(err));
    }
}

bool UdpPort::try_nonblocking_write_(const packet::PacketPtr& pp) {
    if (!config_.enable_non_blocking) {
        return false;
    }

    const packet::UDP& udp = *pp->udp();
    const bool success =
        socket_try_send_to(fd_, pp->buffer().data(), pp->buffer().size(), udp.dst_addr);

    if (success) {
        const int packet_num = ++sent_packets_;
        roc_log(LogTrace,
                "udp port: %s: sent packet non-blocking: num=%d src=%s dst=%s sz=%ld",
                descriptor(), packet_num,
                address::socket_addr_to_str(config_.bind_address).c_str(),
                address::socket_addr_to_str(udp.dst_addr).c_str(),
                (long)pp->buffer().size());
    }

    return success;
}

bool UdpPort::fully_closed_() const {
    if (!handle_initialized_ && !write_sem_initialized_) {
        return true;
    }

    if (closed_) {
        return true;
    }

    return false;
}

void UdpPort::start_closing_() {
    if (fully_closed_()) {
        return;
    }

    roc_log(LogDebug, "udp port: %s: initiating asynchronous close", descriptor());

    if (recv_started_) {
        if (int err = uv_udp_recv_stop(&handle_)) {
            roc_log(LogError, "udp port: %s: uv_udp_recv_stop(): [%s] %s", descriptor(),
                    uv_err_name(err), uv_strerror(err));
        }
        recv_started_ = false;
    }

    if (multicast_group_joined_) {
        leave_multicast_group_();
    }

    if (handle_initialized_ && !uv_is_closing((uv_handle_t*)&handle_)) {
        uv_close((uv_handle_t*)&handle_, close_cb_);
    }

    if (write_sem_initialized_ && !uv_is_closing((uv_handle_t*)&write_sem_)) {
        uv_close((uv_handle_t*)&write_sem_, close_cb_);
    }
}

bool UdpPort::join_multicast_group_() {
    if (!config_.bind_address.multicast()) {
        roc_log(LogError,
                "udp port: %s: can't use multicast group for non-multicast address",
                descriptor());
        return false;
    }

    char host[address::SocketAddr::MaxStrLen];
    if (!config_.bind_address.get_host(host, sizeof(host))) {
        roc_log(LogError, "udp port: %s: can't format address host", descriptor());
        return false;
    }

    if (int err = uv_udp_set_membership(&handle_, host, config_.multicast_interface,
                                        UV_JOIN_GROUP)) {
        roc_log(LogError, "udp port: %s: uv_udp_set_membership(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    roc_log(LogDebug, "udp port: %s: joined multicast group", descriptor());

    return (multicast_group_joined_ = true);
}

void UdpPort::leave_multicast_group_() {
    multicast_group_joined_ = false;

    char host[address::SocketAddr::MaxStrLen];
    if (!config_.bind_address.get_host(host, sizeof(host))) {
        roc_log(LogError, "udp port: %s: can't format address host", descriptor());
        return;
    }

    if (int err = uv_udp_set_membership(&handle_, host, config_.multicast_interface,
                                        UV_LEAVE_GROUP)) {
        roc_log(LogError, "udp port: %s: uv_udp_set_membership(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
    }

    roc_log(LogDebug, "udp port: %s: left multicast group", descriptor());
}

void UdpPort::report_stats_() {
    if (!rate_limiter_.allow()) {
        return;
    }

    const int recv_packets = received_packets_;
    const int sent_packets = sent_packets_;
    const int sent_packets_nb = (sent_packets - sent_packets_blk_);

    roc_log(LogDebug, "udp port: %s: recv=%d send=%d send_nb=%d", descriptor(),
            recv_packets, sent_packets, sent_packets_nb);
}

void UdpPort::format_descriptor(core::StringBuilder& b) {
    b.append_str("<udp");

    b.append_str(" 0x");
    b.append_uint((unsigned long)this, 16);

    b.append_str(" bind=");
    b.append_str(address::socket_addr_to_str(config_.bind_address).c_str());

    b.append_str(">");
}

} // namespace netio
} // namespace roc
