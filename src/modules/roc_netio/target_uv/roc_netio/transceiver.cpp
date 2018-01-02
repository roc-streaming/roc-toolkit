/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/transceiver.h"
#include "roc_core/lock.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"
#include "roc_packet/address_to_str.h"

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
    , task_sem_initialized_(false)
    , num_ports_(0) {
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

    if (num_ports_ != 0) {
        roc_panic("transceiver: %lu port(s) were not removed before calling destructor",
                  (unsigned long)num_ports_);
    }

    close_();

    if (loop_initialized_) {
        // If the thread was never started and joined and thus stop_() was not
        // called, we should manually call it and quickly run the loop to wait
        // all opened handles to be closed. Otherwise, uv_loop_close() will
        // fail with EBUSY.
        if (uv_loop_alive(&loop_)) {
            stop_();
            Transceiver::run(); // non-virtual call from dtor
        }
        if (int err = uv_loop_close(&loop_)) {
            roc_panic("transceiver: uv_loop_close(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    }

    const size_t num_dead_ports = receivers_.size() + senders_.size();

    if (num_dead_ports != 0) {
        roc_panic(
            "transceiver: %lu dead port(s) were not cleaned up before calling destructor",
            (unsigned long)num_dead_ports);
    }
}

bool Transceiver::valid() const {
    return valid_;
}

bool Transceiver::start() {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    core::Lock lock(mutex_);

    if (stopped_) {
        roc_log(LogError, "tranceiver: can't start stopped transceiver");
        return false;
    }

    return Thread::start();
}

void Transceiver::stop() {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    core::Lock lock(mutex_);

    // Ignore subsequent calls, since stop_sem_ may be already closed
    // from event loop thread.
    if (stopped_) {
        return;
    }

    stopped_ = true;

    if (int err = uv_async_send(&stop_sem_)) {
        roc_panic("transceiver: uv_async_send(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }
}

void Transceiver::join() {
    Thread::join();
}

size_t Transceiver::num_ports() const {
    core::Lock lock(mutex_);

    return num_ports_;
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

void Transceiver::remove_port(packet::Address bind_address) {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    Task task;
    task.fn = &Transceiver::remove_port_;
    task.address = &bind_address;
    task.writer = NULL;

    run_task_(task);

    if (!task.result) {
        roc_panic("transceiver: can't remove port %s: unknown port",
                  packet::address_to_str(bind_address).c_str());
    }
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
    self.close_();
    self.process_tasks_();
}

void Transceiver::stop_() {
    for (core::SharedPtr<UDPReceiver> rp = receivers_.front(); rp;
         rp = receivers_.nextof(*rp)) {
        rp->stop();
    }

    for (core::SharedPtr<UDPSender> sp = senders_.front(); sp;
         sp = senders_.nextof(*sp)) {
        sp->stop();
    }
}

void Transceiver::close_() {
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
        core::Lock lock(mutex_);

        const bool running = joinable();

        if (!running || stopped_) {
            // If a stop was scheduled, ensure it have finished.
            if (running) {
                mutex_.unlock();
                join();
                mutex_.lock();
            }

            // There is no background thread, so execute task in-place.
            task.result = (this->*(task.fn))(task);
            return;
        }

        tasks_.push_back(task);

        if (int err = uv_async_send(&task_sem_)) {
            roc_panic("transceiver: uv_async_send(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    }

    task.done.pend();
}

void Transceiver::process_tasks_() {
    core::Lock lock(mutex_);

    while (Task* task = tasks_.front()) {
        tasks_.remove(*task);
        task->result = (this->*(task->fn))(*task);
        task->done.post();
    }
}

bool Transceiver::add_udp_receiver_(Task& task) {
    if (stopped_) {
        roc_log(LogError, "transceiver: can't add port %s: transceiver is stopped",
                packet::address_to_str(*task.address).c_str());
        return false;
    }

    if (has_port_(*task.address)) {
        roc_log(LogError, "transceiver: can't add port %s: duplicate address",
                packet::address_to_str(*task.address).c_str());
        return false;
    }

    core::SharedPtr<UDPReceiver> rp = new (allocator_)
        UDPReceiver(loop_, *task.writer, packet_pool_, buffer_pool_, allocator_);

    if (!rp) {
        roc_log(LogError, "transceiver: can't add port %s: can't allocate receiver",
                packet::address_to_str(*task.address).c_str());
        return false;
    }

    if (!rp->start(*task.address)) {
        roc_log(LogError, "transceiver: can't add port %s: can't start receiver",
                packet::address_to_str(*task.address).c_str());
        return false;
    }

    receivers_.push_back(*rp);
    num_ports_++;

    return true;
}

bool Transceiver::add_udp_sender_(Task& task) {
    if (stopped_) {
        roc_log(LogError, "transceiver: can't add port %s: transceiver is stopped",
                packet::address_to_str(*task.address).c_str());
        return false;
    }

    if (has_port_(*task.address)) {
        roc_log(LogError, "transceiver: can't add port %s: duplicate address",
                packet::address_to_str(*task.address).c_str());
        return false;
    }

    core::SharedPtr<UDPSender> sp = new (allocator_) UDPSender(loop_, allocator_);

    if (!sp) {
        roc_log(LogError, "transceiver: can't add port %s: can't allocate sender",
                packet::address_to_str(*task.address).c_str());
        return false;
    }

    if (!sp->start(*task.address)) {
        roc_log(LogError, "transceiver: can't add port %s: can't start sender",
                packet::address_to_str(*task.address).c_str());
        return false;
    }

    senders_.push_back(*sp);
    num_ports_++;

    task.writer = sp.get();
    return true;
}

bool Transceiver::remove_port_(Task& task) {
    for (core::SharedPtr<UDPReceiver> rp = receivers_.front(); rp;
         rp = receivers_.nextof(*rp)) {
        if (rp->address() == *task.address) {
            rp->remove(receivers_);
            num_ports_--;
            return true;
        }
    }

    for (core::SharedPtr<UDPSender> sp = senders_.front(); sp;
         sp = senders_.nextof(*sp)) {
        if (sp->address() == *task.address) {
            sp->remove(senders_);
            num_ports_--;
            return true;
        }
    }

    return false;
}

bool Transceiver::has_port_(const packet::Address& address) const {
    for (core::SharedPtr<UDPReceiver> rp = receivers_.front(); rp;
         rp = receivers_.nextof(*rp)) {
        if (rp->address() == address) {
            return true;
        }
    }

    for (core::SharedPtr<UDPSender> sp = senders_.front(); sp;
         sp = senders_.nextof(*sp)) {
        if (sp->address() == address) {
            return true;
        }
    }

    return false;
}

} // namespace netio
} // namespace roc
