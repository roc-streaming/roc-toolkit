/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/transceiver.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"
#include "roc_netio/handle.h"
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
    , num_ports_(0)
    , cond_(mutex_) {
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

    stop_handle_.data = this;
    stop_handle_.fn = remove_port_cb_;

    valid_ = Thread::start();
}

Transceiver::~Transceiver() {
    if (joinable()) {
        roc_panic("transceiver: thread is not joined before calling destructor");
    }

    roc_panic_if(num_ports_);

    roc_panic_if(receivers_.size());
    roc_panic_if(senders_.size());

    if (loop_initialized_) {
        // If the thread was never started and joined and thus stop() was not
        // called, we should manually run the loop to wait all opened handles
        // to be closed. Otherwise, uv_loop_close() will fail with EBUSY.
        if (uv_loop_alive(&loop_)) {
            close_();

            Transceiver::run(); // non-virtual call from dtor
        }

        roc_panic_if(task_sem_initialized_);
        roc_panic_if(stop_sem_initialized_);

        if (int err = uv_loop_close(&loop_)) {
            roc_panic("transceiver: uv_loop_close(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    }
}

bool Transceiver::valid() const {
    return valid_;
}

void Transceiver::stop() {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    stop_();
    Thread::join();
}

size_t Transceiver::num_ports() const {
    core::Mutex::Lock lock(mutex_);

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

    wait_port_removed_(bind_address);
}

void Transceiver::run() {
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
    self.stop_all_();
    self.close_();
    self.process_tasks_();
}

void Transceiver::close_cb_(uv_handle_t* handle) {
    roc_panic_if_not(handle);

    Transceiver& self = *(Transceiver*)handle->data;

    core::Mutex::Lock lock(self.mutex_);

    if (handle == (uv_handle_t*)&self.stop_sem_) {
        self.stop_sem_initialized_ = false;
    } else {
        self.task_sem_initialized_ = false;
    }

    if (self.stop_sem_initialized_ || self.task_sem_initialized_) {
        return;
    }

    self.cond_.broadcast();
}

void Transceiver::remove_port_cb_(void* data, packet::Address& address) {
    roc_log(LogDebug, "transceiver: removed port %s",
            packet::address_to_str(address).c_str());

    Transceiver& self = *(Transceiver*)data;

    core::Mutex::Lock lock(self.mutex_);

    core::SharedPtr<UDPReceiver> rp = self.get_receiver_(address);
    if (!rp) {
        core::SharedPtr<UDPSender> sp = self.get_sender_(address);
        if (!sp) {
            roc_panic("transceiver: can't remove unknown port %s",
                      packet::address_to_str(address).c_str());
        } else {
            self.senders_.remove(*sp);
        }
    } else {
        self.receivers_.remove(*rp);
    }

    self.num_ports_--;

    self.cond_.broadcast();
}

void Transceiver::stop_() {
    core::Mutex::Lock lock(mutex_);

    if (stopped_) {
        return;
    }

    if (int err = uv_async_send(&stop_sem_)) {
        roc_panic("transceiver: uv_async_send(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }

    wait_stopped_();
    wait_closed_();

    stopped_ = true;
}

void Transceiver::wait_stopped_() {
    while (receivers_.size() || senders_.size()) {
        cond_.wait();
    }
}

void Transceiver::wait_closed_() {
    while (task_sem_initialized_ || stop_sem_initialized_) {
        cond_.wait();
    }
}

void Transceiver::stop_all_() {
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
        uv_close((uv_handle_t*)&task_sem_, close_cb_);
    }

    if (stop_sem_initialized_) {
        uv_close((uv_handle_t*)&stop_sem_, close_cb_);
    }
}

void Transceiver::run_task_(Task& task) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(stopped_);

    tasks_.push_back(task);

    if (int err = uv_async_send(&task_sem_)) {
        roc_panic("transceiver: uv_async_send(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }

    while (!task.done) {
        cond_.wait();
    }
}

void Transceiver::process_tasks_() {
    core::Mutex::Lock lock(mutex_);

    while (Task* task = tasks_.front()) {
        tasks_.remove(*task);
        task->execute(*this);
    }

    cond_.broadcast();
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

    core::SharedPtr<UDPReceiver> rp = new (allocator_) UDPReceiver(
        loop_, stop_handle_, *task.writer, packet_pool_, buffer_pool_, allocator_);

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

    core::SharedPtr<UDPSender> sp =
        new (allocator_) UDPSender(loop_, stop_handle_, allocator_);

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

bool Transceiver::has_port_(const packet::Address& address) const {
    core::SharedPtr<UDPReceiver> rp = get_receiver_(address);
    if (rp) {
        return true;
    }

    core::SharedPtr<UDPSender> sp = get_sender_(address);
    if (sp) {
        return true;
    }

    return false;
}

bool Transceiver::remove_port_(Task& task) {
    roc_log(LogDebug, "transceiver: removing port %s",
            packet::address_to_str(*task.address).c_str());

    core::SharedPtr<UDPReceiver> rp = get_receiver_(*task.address);
    if (rp) {
        rp->stop();
        return true;
    }

    core::SharedPtr<UDPSender> sp = get_sender_(*task.address);
    if (sp) {
        sp->stop();
        return true;
    }

    return false;
}

void Transceiver::wait_port_removed_(const packet::Address& address) const {
    core::Mutex::Lock lock(mutex_);

    while (has_port_(address)) {
        cond_.wait();
    }
}

core::SharedPtr<UDPReceiver>
Transceiver::get_receiver_(const packet::Address& address) const {
    for (core::SharedPtr<UDPReceiver> pp = receivers_.front(); pp;
         pp = receivers_.nextof(*pp)) {
        if (pp->address() == address) {
            return pp;
        }
    }

    return NULL;
}

core::SharedPtr<UDPSender>
Transceiver::get_sender_(const packet::Address& address) const {
    for (core::SharedPtr<UDPSender> pp = senders_.front(); pp;
         pp = senders_.nextof(*pp)) {
        if (pp->address() == address) {
            return pp;
        }
    }

    return NULL;
}

} // namespace netio
} // namespace roc
