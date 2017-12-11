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
    , valid_(false)
    , stopped_(false)
    , loop_initialized_(false)
    , stop_sem_initialized_(false)
    , task_sem_initialized_(false) {
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

    if (int err = uv_async_init(&loop_, &task_sem_, task_sem_cb_)) {
        roc_log(LogError, "transceiver: uv_async_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return;
    }
    task_sem_.data = this;
    task_sem_initialized_ = true;

    valid_ = true;
}

Transceiver::~Transceiver() {
    if (joinable()) {
        roc_panic("transceiver: thread is not joined before calling destructor");
    }

    close_sem_();

    if (loop_initialized_) {
        // If the thread was never started and joined and thus stop_() was not
        // called, we should manually call it and quickly run the loop to wait
        // all opened handles to be closed. Otherwise, uv_loop_close() will
        // fail with EBUSY.
        if (uv_loop_alive(&loop_)) {
            stopped_ = true;
            stop_();
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

void Transceiver::start() {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    core::Mutex::Lock lock(mutex_);

    Thread::start();
}

void Transceiver::stop() {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    {
        core::Mutex::Lock lock(mutex_);

        // Ignore subsequent calls, since stop_sem_ may be already closed
        // from event loop thread.
        if (stopped_) {
            return;
        }

        stopped_ = true;
    }

    if (int err = uv_async_send(&stop_sem_)) {
        roc_panic("transceiver: uv_async_send(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }
}

void Transceiver::join() {
    Thread::join();
}

bool Transceiver::add_udp_receiver(packet::Address& bind_address,
                                   packet::IWriter& writer) {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    Task task;
    task.fn = &Transceiver::add_udp_receiver_;
    task.address = &bind_address;
    task.writer = &writer;

    run_task_(task);

    return task.result;
}

packet::IWriter* Transceiver::add_udp_sender(packet::Address& bind_address) {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    Task task;
    task.fn = &Transceiver::add_udp_sender_;
    task.address = &bind_address;
    task.writer = NULL;

    run_task_(task);

    return task.writer;
}

void Transceiver::run() {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    roc_log(LogDebug, "transceiver: starting event loop");

    int err = uv_run(&loop_, UV_RUN_DEFAULT);
    if (err != 0) {
        roc_log(LogInfo, "transceiver: uv_run() returned non-zero");
    }

    roc_log(LogDebug, "transceiver: finishing event loop");
}

void Transceiver::task_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);

    Transceiver& self = *(Transceiver*)handle->data;
    self.process_tasks_();
}

void Transceiver::stop_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);

    Transceiver& self = *(Transceiver*)handle->data;

    self.stop_();
    self.close_sem_();
}

void Transceiver::stop_() {
    // cancel enqueued tasks
    process_tasks_();

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
    if (task_sem_initialized_) {
        uv_close((uv_handle_t*)&task_sem_, NULL);
        task_sem_initialized_ = false;
    }

    if (stop_sem_initialized_) {
        uv_close((uv_handle_t*)&stop_sem_, NULL);
        stop_sem_initialized_ = false;
    }
}

void Transceiver::run_task_(Task& task) {
    {
        core::Mutex::Lock lock(mutex_);

        // Ignore calls after stop(), since task_sem_ may be already closed
        // from event loop thread.
        if (stopped_) {
            return;
        }

        // There is no background thread, execute task in-place.
        if (!joinable()) {
            task.result = (this->*(task.fn))(task);
            return;
        }

        tasks_.push_back(task);
    }

    if (int err = uv_async_send(&task_sem_)) {
        roc_panic("transceiver: uv_async_send(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }

    task.done.pend();
}

void Transceiver::process_tasks_() {
    core::Mutex::Lock lock(mutex_);

    while (Task* task = tasks_.front()) {
        if (!stopped_) {
            task->result = (this->*(task->fn))(*task);
        }
        tasks_.remove(*task);
        task->done.post();
    }
}

bool Transceiver::add_udp_receiver_(Task& task) {
    core::SharedPtr<UDPReceiver> rp = new (allocator_)
        UDPReceiver(loop_, *task.writer, packet_pool_, buffer_pool_, allocator_);

    if (!rp) {
        roc_log(LogError, "transceiver: can't allocate udp receiver");
        return false;
    }

    if (!rp->start(*task.address)) {
        roc_log(LogError, "transceiver: can't start udp receiver");
        return false;
    }

    receivers_.push_back(*rp);

    return true;
}

bool Transceiver::add_udp_sender_(Task& task) {
    core::SharedPtr<UDPSender> sp = new (allocator_) UDPSender(loop_, allocator_);

    if (!sp) {
        roc_log(LogError, "transceiver: can't allocate udp sender");
        return false;
    }

    if (!sp->start(*task.address)) {
        roc_log(LogError, "transceiver: can't start udp sender");
        return false;
    }

    senders_.push_back(*sp);
    task.writer = sp.get();

    return true;
}

} // namespace netio
} // namespace roc
