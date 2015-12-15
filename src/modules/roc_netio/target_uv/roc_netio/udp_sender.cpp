/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/log.h"

#include "roc_datagram/address_to_str.h"

#include "roc_netio/inet_address.h"
#include "roc_netio/udp_sender.h"

namespace roc {
namespace netio {

UDPSender::UDPSender()
    : loop_(NULL)
    , number_(0) {
}

UDPSender::~UDPSender() {
    if (loop_) {
        roc_panic("udp sender: detach() was not called before destructor");
    }
}

void UDPSender::attach(uv_loop_t& loop) {
    core::SpinMutex::Lock lock(mutex_);

    roc_log(LOG_TRACE, "udp sender: attaching to event loop");

    roc_panic_if(loop_);

    loop_ = &loop;

    if (int err = uv_async_init(loop_, &async_, async_cb_)) {
        roc_panic("udp sender: uv_async_init(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }

    async_.data = this;
}

void UDPSender::detach(uv_loop_t& loop) {
    core::SpinMutex::Lock lock(mutex_);

    roc_log(LOG_TRACE, "udp sender: detaching from event loop");

    roc_panic_if(loop_ != &loop);

    for (size_t n = 0; n < ports_.size(); n++) {
        close_port_(ports_[n]);
    }

    uv_close((uv_handle_t*)&async_, NULL);

    loop_ = NULL;
}

bool UDPSender::add_port(const datagram::Address& address) {
    roc_log(LOG_DEBUG, "udp sender: adding port %s",
            datagram::address_to_str(address).c_str());

    if (!loop_) {
        roc_panic("udp sender: not attached to event loop");
    }

    if (ports_.size() == ports_.max_size()) {
        roc_panic("udp sender: can't add more than %ld ports", (long)ports_.max_size());
    }

    Port* port = new (ports_.allocate()) Port;

    port->address = address;

    if (!open_port_(*port)) {
        roc_log(LOG_ERROR, "udp sender: can't add port %s",
                datagram::address_to_str(address).c_str());

        ports_.resize(ports_.size() - 1);
        return false;
    }

    return true;
}

bool UDPSender::open_port_(Port& port) {
    roc_log(LOG_TRACE, "udp sender: opening port %s",
            datagram::address_to_str(port.address).c_str());

    if (int err = uv_udp_init(loop_, &port.handle)) {
        roc_log(LOG_ERROR, "udp sender: uv_udp_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    port.handle.data = this;

    sockaddr_in inet_addr;
    to_inet_address(port.address, inet_addr);

    if (int err = uv_udp_bind(&port.handle, (sockaddr*)&inet_addr, UV_UDP_REUSEADDR)) {
        roc_log(LOG_ERROR, "udp sender: uv_udp_bind(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return false;
    }

    return true;
}

void UDPSender::close_port_(Port& port) {
    if (uv_is_closing((uv_handle_t*)&port.handle)) {
        return;
    }

    roc_log(LOG_TRACE, "udp sender: closing port %s",
            datagram::address_to_str(port.address).c_str());

    uv_close((uv_handle_t*)&port.handle, NULL);
}

UDPSender::Port* UDPSender::find_port_(const datagram::Address& address) {
    for (size_t n = 0; n < ports_.size(); n++) {
        if (ports_[n].address == address) {
            return &ports_[n];
        }
    }

    return NULL;
}

void UDPSender::write(const datagram::IDatagramPtr& dgm) {
    if (!dgm) {
        roc_panic("udp sender: attempting to write null datagram");
    }

    if (dgm->type() != UDPDatagram::Type) {
        roc_panic("udp sender: attempting to write datagram of wrong type"
                  " (use Transceiver::udp_composer() to create datagrams"
                  " suitable for this sender)");
    }

    if (!dgm->buffer()) {
        roc_log(LOG_TRACE, "udp sender: ignoring datagram with empty buffer");
        return;
    }

    UDPDatagram& udp_datagram = static_cast<UDPDatagram&>(*dgm);

    core::SpinMutex::Lock lock(mutex_);

    // uv_async_t may be closed in detach() from event loop thread.
    // In this case, loop_ will be set to null.
    if (!loop_) {
        roc_log(LOG_ERROR, "udp sender:"
                           " dropping datagram, not attached to event loop");
        return;
    }

    list_.append(udp_datagram);

    if (int err = uv_async_send(&async_)) {
        roc_panic("udp sender: uv_async_send(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }
}

UDPDatagramPtr UDPSender::read_() {
    core::SpinMutex::Lock lock(mutex_);

    UDPDatagramPtr dgm = list_.front();
    if (dgm) {
        list_.remove(*dgm);
    }

    return dgm;
}

void UDPSender::async_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);

    UDPSender& self = *(UDPSender*)handle->data;

    while (UDPDatagramPtr dgm = self.read_()) {
        const core::IByteBufferConstSlice& buffer = dgm->buffer();

        self.number_++;

        roc_log(LOG_FLOOD, "udp sender: sending datagram:"
                           " num=%u src=%s dst=%s sz=%ld",
                self.number_,                                      //
                datagram::address_to_str(dgm->sender()).c_str(),   //
                datagram::address_to_str(dgm->receiver()).c_str(), //
                (long)buffer.size());

        Port* port = self.find_port_(dgm->sender());
        if (!port) {
            roc_log(LOG_ERROR,
                    "udp sender: dropping datagram, no port added for sender address %s"
                    " (use Transceiver::add_udp_sender() to register sender address)",
                    datagram::address_to_str(dgm->sender()).c_str());
            continue;
        }

        sockaddr_in inet_addr;
        to_inet_address(dgm->receiver(), inet_addr);

        uv_buf_t buf;
        buf.base = (char*)const_cast<uint8_t*>(buffer.data());
        buf.len = buffer.size();

        if (int err = uv_udp_send(&dgm->request(), &port->handle, &buf, 1,
                                  (sockaddr*)&inet_addr, send_cb_)) {
            roc_log(LOG_ERROR, "udp sender: uv_udp_send(): [%s] %s", uv_err_name(err),
                    uv_strerror(err));
            continue;
        }

        // Will be decremented in send_cb_().
        dgm->incref();
    }
}

void UDPSender::send_cb_(uv_udp_send_t* req, int status) {
    roc_panic_if_not(req);

    // To preventr leak, capture smart pointer before returning.
    UDPDatagramPtr dgm = UDPDatagram::container_of(req);

    // One reference for incref() called from async_cb_(),
    // one more for local smart pointer.
    roc_panic_if(dgm->getref() < 2);

    // Decrement reference counter incremented in async_cb_().
    dgm->decref();

    if (status < 0) {
        roc_log(LOG_ERROR, "udp sender:"
                           " can't send datagram: src=%s dst=%s sz=%ld: [%s] %s",
                datagram::address_to_str(dgm->sender()).c_str(),
                datagram::address_to_str(dgm->receiver()).c_str(),
                (long)dgm->buffer().size(), uv_err_name(status), uv_strerror(status));
    }
}

} // namespace netio
} // namespace roc
