/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/transceiver.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace netio {

Transceiver::Transceiver(packet::PacketPool& packet_pool,
                         core::BufferPool<uint8_t>& buffer_pool,
                         core::IAllocator& allocator)
    : packet_pool_(packet_pool)
    , buffer_pool_(buffer_pool)
    , allocator_(allocator)
    , loop_initialized_(false)
    , stop_sem_initialized_(false)
    , valid_(false) {
    if (int err = uv_loop_init(&loop_)) {
        roc_log(LogError, "transceiver: uv_loop_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return;
    }
    loop_initialized_ = true;

    if (int err = uv_async_init(&loop_, &stop_sem_, stop_sem_cb_)) {
        roc_log(LogError, "transceiver: uv_async_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return;
    }
    stop_sem_.data = this;
    stop_sem_initialized_ = true;

    valid_ = true;
}

Transceiver::~Transceiver() {
    if (joinable()) {
        roc_panic("transceiver: thread is not joined before calling destructor");
    }

    close_sem_();

    if (loop_initialized_) {
        // If there are opened handles (probably because thread was never started and
        // joined and thus close_() wasn't called), we should schedule close and
        // quickly run the loop to wait all opened handles to be closed. Otherwise,
        // uv_loop_close() may fail with EBUSY.
        if (uv_loop_alive(&loop_)) {
            stop_io_();
            Transceiver::run(); // non-virtual call from dtor
        }
        if (int err = uv_loop_close(&loop_)) {
            roc_panic("transceiver: uv_loop_close(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    }
}

bool Transceiver::valid() const {
    return valid_;
}

bool Transceiver::add_udp_receiver(packet::Address& bind_address,
                                   packet::IWriter& writer) {
    if (joinable()) {
        roc_panic("transceiver: can't call add_udp_receiver() when thread is running");
    }

    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    core::SharedPtr<UDPReceiver> rp = new (allocator_)
        UDPReceiver(loop_, writer, packet_pool_, buffer_pool_, allocator_);

    if (!rp) {
        roc_log(LogError, "transceiver: can't allocate udp receiver");
        return false;
    }

    if (!rp->start(bind_address)) {
        roc_log(LogError, "transceiver: can't start udp receiver");
        return false;
    }

    receivers_.push_back(*rp);
    return true;
}

packet::IWriter* Transceiver::add_udp_sender(packet::Address& bind_address) {
    if (joinable()) {
        roc_panic("transceiver: can't call add_udp_sender() when thread is running");
    }

    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    core::SharedPtr<UDPSender> sp = new (allocator_) UDPSender(loop_, allocator_);

    if (!sp) {
        roc_log(LogError, "transceiver: can't allocate udp sender");
        return NULL;
    }

    if (!sp->start(bind_address)) {
        roc_log(LogError, "transceiver: can't start udp sender");
        return NULL;
    }

    senders_.push_back(*sp);
    return sp.get();
}

void Transceiver::run() {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    roc_log(LogInfo, "transceiver: starting event loop");

    int err = uv_run(&loop_, UV_RUN_DEFAULT);
    if (err != 0) {
        roc_log(LogInfo, "transceiver: uv_run() returned non-zero");
    }

    roc_log(LogInfo, "transceiver: finishing event loop");
}

void Transceiver::stop() {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    // Ignore subsequent calls, since after first call uv_async_t
    // may be already closed from event loop thread.
    if (stopped_.test_and_set() != 0) {
        return;
    }

    if (int err = uv_async_send(&stop_sem_)) {
        roc_panic("transceiver: uv_async_send(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }
}

void Transceiver::stop_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);

    Transceiver& self = *(Transceiver*)handle->data;
    self.stop_io_();
    self.close_sem_();
}

void Transceiver::stop_io_() {
    core::SharedPtr<UDPReceiver> rp;
    for (rp = receivers_.front(); rp; rp = receivers_.nextof(*rp)) {
        rp->stop();
    }

    core::SharedPtr<UDPSender> sp;
    for (sp = senders_.front(); sp; sp = senders_.nextof(*sp)) {
        sp->stop();
    }
}

void Transceiver::close_sem_() {
    if (stop_sem_initialized_) {
        uv_close((uv_handle_t*)&stop_sem_, NULL);
        stop_sem_initialized_ = false;
    }
}

} // namespace netio
} // namespace roc
