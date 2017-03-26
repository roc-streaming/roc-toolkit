/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/helpers.h"
#include "roc_core/panic.h"
#include "roc_core/log.h"

#include "roc_datagram/address_to_str.h"

#include "roc_netio/inet_address.h"
#include "roc_netio/udp_receiver.h"

namespace roc {
namespace netio {

UDPReceiver::UDPReceiver(core::IByteBufferComposer& buf_composer,
                         UDPComposer& dgm_composer)
    : loop_(NULL)
    , buf_composer_(buf_composer)
    , dgm_composer_(dgm_composer)
    , number_(0) {
}

UDPReceiver::~UDPReceiver() {
    if (loop_) {
        roc_panic("udp receiver: detach() was not called before destructor");
    }
}

void UDPReceiver::attach(uv_loop_t& loop) {
    roc_log(LogDebug, "udp receiver: attaching to event loop");

    roc_panic_if(loop_);

    loop_ = &loop;
}

void UDPReceiver::detach(uv_loop_t& loop) {
    roc_log(LogDebug, "udp receiver: detaching from event loop");

    roc_panic_if(loop_ != &loop);

    for (size_t n = 0; n < ports_.size(); n++) {
        close_port_(ports_[n]);
    }

    loop_ = NULL;
}

bool UDPReceiver::add_port(const datagram::Address& address,
                           datagram::IDatagramWriter& writer) {
    roc_log(LogInfo, "udp receiver: adding port %s",
            datagram::address_to_str(address).c_str());

    if (!loop_) {
        roc_panic("udp receiver: not attached to event loop");
    }

    if (ports_.size() == ports_.max_size()) {
        roc_panic("udp receiver: can't add more than %ld ports", (long)ports_.max_size());
    }

    Port* port = new (ports_.allocate()) Port;

    port->address = address;
    port->writer = &writer;

    if (!open_port_(*port)) {
        roc_log(LogError, "udp receiver: can't add port %s",
                datagram::address_to_str(address).c_str());

        ports_.resize(ports_.size() - 1);
        return false;
    }

    return true;
}

bool UDPReceiver::open_port_(Port& port) {
    roc_log(LogDebug, "udp receiver: opening port %s",
            datagram::address_to_str(port.address).c_str());

    if (int err = uv_udp_init(loop_, &port.handle)) {
        roc_log(LogError, "udp receiver: uv_udp_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    port.handle.data = this;

    sockaddr_in inet_addr;
    to_inet_address(port.address, inet_addr);

    if (int err = uv_udp_bind(&port.handle, (sockaddr*)&inet_addr, UV_UDP_REUSEADDR)) {
        roc_log(LogError, "udp receiver: uv_udp_bind(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    if (int err = uv_udp_recv_start(&port.handle, alloc_cb_, recv_cb_)) {
        roc_log(LogError, "udp receiver: uv_udp_recv_start(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    return true;
}

void UDPReceiver::close_port_(Port& port) {
    if (uv_is_closing((uv_handle_t*)&port.handle)) {
        return;
    }

    roc_log(LogDebug, "udp receiver: closing port %s",
            datagram::address_to_str(port.address).c_str());

    if (int err = uv_udp_recv_stop(&port.handle)) {
        roc_log(LogError, "udp receiver: uv_udp_recv_stop(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
    }

    uv_close((uv_handle_t*)&port.handle, NULL);
}

void UDPReceiver::alloc_cb_(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
    roc_panic_if_not(handle);
    roc_panic_if_not(buf);

    roc_log(LogTrace, "udp receiver: allocating buffer: size=%ld", (long)size);

    UDPReceiver& self = *(UDPReceiver*)handle->data;

    core::IByteBufferPtr bp = self.buf_composer_.compose();
    if (!bp) {
        roc_log(LogError, "udp receiver: can't get buffer from pool");

        buf->base = NULL;
        buf->len = 0;

        return;
    }

    if (size > bp->max_size()) {
        roc_log(LogTrace, "udp receiver: truncating buffer size:"
                           " suggested=%ld max=%ld",
                (long)size, (long)bp->max_size());

        size = bp->max_size();
    }

    bp->set_size(size);
    bp->incref(); // Will be decremented in recv_cb_().

    buf->base = (char*)bp->data();
    buf->len = size;
}

void UDPReceiver::recv_cb_(uv_udp_t* handle,
                           ssize_t nread,
                           const uv_buf_t* buf,
                           const sockaddr* inet_addr,
                           unsigned flags) {
    roc_panic_if_not(handle);
    roc_panic_if_not(buf);

    UDPReceiver& self = *(UDPReceiver*)handle->data;
    self.number_++;

    const Port* port = ROC_CONTAINER_OF(handle, Port, handle);

    datagram::Address sender_addr;
    if (inet_addr) {
        from_inet_address(*(const sockaddr_in*)inet_addr, sender_addr);
    }

    roc_log(LogTrace, "udp receiver: got datagram: num=%u src=%s dst=%s nread=%ld",
            self.number_,                                    //
            datagram::address_to_str(sender_addr).c_str(),   //
            datagram::address_to_str(port->address).c_str(), //
            (long)nread);

    // To preventr leak, capture smart pointer before returning.
    core::IByteBufferPtr bp = self.buf_composer_.container_of((uint8_t*)buf->base);

    // Recheck size.
    roc_panic_if(bp->size() != buf->len);

    // One reference for incref() called from alloc_cb_(),
    // one more for local smart pointer.
    roc_panic_if(bp->getref() != 2);

    // Decrement reference counter incremented in alloc_cb_().
    bp->decref();

    if (nread < 0) {
        roc_log(LogError, "udp receiver: network error: num=%u src=%s dst=%s nread=%ld",
                self.number_,                                    //
                datagram::address_to_str(sender_addr).c_str(),   //
                datagram::address_to_str(port->address).c_str(), //
                (long)nread);
        return;
    }

    if (nread == 0) {
        if (inet_addr == NULL) {
            // no more data for now
        } else {
            roc_log(LogTrace, "udp receiver: empty datagram: num=%u src=%s dst=%s",
                    self.number_,                                  //
                    datagram::address_to_str(sender_addr).c_str(), //
                    datagram::address_to_str(port->address).c_str());
        }
        return;
    }

    if (inet_addr == NULL) {
        roc_panic("udp receiver: unexpected null source address");
    }

    if (flags & UV_UDP_PARTIAL) {
        roc_log(LogDebug, "udp receiver:"
                           " ignoring partial read: num=%u src=%s dst=%s nread=%ld",
                self.number_,                                    //
                datagram::address_to_str(sender_addr).c_str(),   //
                datagram::address_to_str(port->address).c_str(), //
                (long)nread);
        return;
    }

    if ((size_t)nread > bp->size()) {
        roc_panic("udp receiver: unexpected buffer size (got %ld, max %ld)", (long)nread,
                  (long)bp->size());
    }

    bp->set_size((size_t)nread);

    datagram::IDatagramPtr dgm = self.dgm_composer_.compose();
    if (!dgm) {
        roc_log(LogError, "udp receiver: composer returned null");
        return;
    }

    dgm->set_receiver(port->address);
    dgm->set_sender(sender_addr);
    dgm->set_buffer(*bp);

    port->writer->write(dgm);
}

} // namespace netio
} // namespace roc
