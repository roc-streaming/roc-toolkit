/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_core/panic.h"

#include "roc_netio/transceiver.h"

namespace roc {
namespace netio {

Transceiver::Transceiver(core::IByteBufferComposer& buf_composer,
                         core::IPool<UDPDatagram>& datagram_pool)
    : udp_composer_(datagram_pool)
    , udp_receiver_(buf_composer, udp_composer_)
    , udp_sender_() {
    //
    if (int err = uv_loop_init(&loop_)) {
        roc_panic("transceiver: uv_loop_init(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }

    if (int err = uv_async_init(&loop_, &async_, async_cb_)) {
        roc_panic("transceiver: uv_async_init(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }

    async_.data = this;

    udp_receiver_.attach(loop_);
    udp_sender_.attach(loop_, async_);
}

Transceiver::~Transceiver() {
    if (joinable()) {
        roc_panic("transceiver: thread is not joined before calling destructor");
    }

    // If there are opened handles (probably because thread was never started and
    // joined and thus close_() wasn't called), we should schedule close and
    // quickly run the loop to wait all opened handles to be closed. Otherwise,
    // uv_loop_close() may fail with EBUSY.
    if (uv_loop_alive(&loop_)) {
        close_();
        Transceiver::run(); // non-virtual call from dtor
    }

    if (int err = uv_loop_close(&loop_)) {
        roc_panic("transceiver: uv_loop_close(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }
}

bool Transceiver::add_udp_receiver(const datagram::Address& address,
                                   datagram::IDatagramWriter& writer) {
    if (joinable()) {
        roc_panic("transceiver: can't call add_udp_receiver() when thread is running");
    }

    return udp_receiver_.add_port(address, writer);
}

bool Transceiver::add_udp_sender(const datagram::Address& address) {
    if (joinable()) {
        roc_panic("transceiver: can't call add_udp_sender() when thread is running");
    }

    return udp_sender_.add_port(address);
}

datagram::IDatagramComposer& Transceiver::udp_composer() {
    return udp_composer_;
}

datagram::IDatagramWriter& Transceiver::udp_sender() {
    return udp_sender_;
}

void Transceiver::run() {
    roc_log(LogInfo, "transceiver: starting event loop");

    int err = uv_run(&loop_, UV_RUN_DEFAULT);
    if (err != 0) {
        roc_log(LogInfo, "transceiver: uv_run() returned non-zero");
    }

    roc_log(LogInfo, "transceiver: finishing event loop");
}

void Transceiver::stop() {
    // Ignore subsequent calls, since after first call uv_async_t
    // may be already closed from event loop thread.
    if (stop_.test_and_set() != 0) {
        return;
    }

    if (int err = uv_async_send(&async_)) {
        roc_panic("transceiver: uv_async_send(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }
}

void Transceiver::async_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);

    Transceiver& self = *(Transceiver*)handle->data;
    self.close_();
}

void Transceiver::close_() {
    // Close all opened handles. This only *schedules* closing; when
    // all handles are actually closed, uv_run() returns.
    udp_receiver_.detach(loop_);
    udp_sender_.detach(loop_);

    uv_close((uv_handle_t*)&async_, NULL);
}

} // namespace netio
} // namespace roc
