/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/transceiver.h"
#include "roc_address/socket_addr_to_str.h"
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
    , started_(false)
    , loop_initialized_(false)
    , stop_sem_initialized_(false)
    , task_sem_initialized_(false)
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

    started_ = Thread::start();
}

Transceiver::~Transceiver() {
    if (started_) {
        if (int err = uv_async_send(&stop_sem_)) {
            roc_panic("transceiver: uv_async_send(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    } else {
        close_sems_();
    }

    if (loop_initialized_) {
        if (started_) {
            Thread::join();
        } else {
            // If the thread was never started we should manually run the loop to
            // wait all opened handles to be closed. Otherwise, uv_loop_close()
            // will fail with EBUSY.
            Transceiver::run(); // non-virtual call from dtor
        }

        if (int err = uv_loop_close(&loop_)) {
            roc_panic("transceiver: uv_loop_close(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    }

    roc_panic_if(joinable());
    roc_panic_if(open_ports_.size());
    roc_panic_if(closing_ports_.size());
    roc_panic_if(task_sem_initialized_);
    roc_panic_if(stop_sem_initialized_);
}

bool Transceiver::valid() const {
    return started_;
}

size_t Transceiver::num_ports() const {
    core::Mutex::Lock lock(mutex_);

    return open_ports_.size();
}

bool Transceiver::add_udp_receiver(address::SocketAddr& bind_address,
                                   packet::IWriter& writer) {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    Task task;
    task.fn = &Transceiver::add_udp_receiver_;
    task.address = &bind_address;
    task.writer = &writer;

    run_task_(task);

    if (!task.result) {
        if (task.port) {
            wait_port_closed_(*task.port);
        }
    }

    return task.result;
}

packet::IWriter* Transceiver::add_udp_sender(address::SocketAddr& bind_address) {
    if (!valid()) {
        roc_panic("transceiver: can't use invalid transceiver");
    }

    Task task;
    task.fn = &Transceiver::add_udp_sender_;
    task.address = &bind_address;
    task.writer = NULL;

    run_task_(task);

    if (!task.result) {
        if (task.port) {
            wait_port_closed_(*task.port);
        }
    }

    return task.writer;
}

void Transceiver::remove_port(address::SocketAddr bind_address) {
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
                  address::socket_addr_to_str(bind_address).c_str());
    } else {
        roc_panic_if_not(task.port);
        wait_port_closed_(*task.port);
    }
}

void Transceiver::handle_closed(BasicPort& port) {
    core::Mutex::Lock lock(mutex_);

    for (core::SharedPtr<BasicPort> pp = closing_ports_.front(); pp;
         pp = closing_ports_.nextof(*pp)) {
        if (pp.get() != &port) {
            continue;
        }

        roc_log(LogDebug, "transceiver: asynchronous close finished: port %s",
                address::socket_addr_to_str(port.address()).c_str());

        closing_ports_.remove(*pp);
        cond_.broadcast();

        break;
    }
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
    self.async_close_ports_();
    self.close_sems_();
    self.process_tasks_();
}

void Transceiver::async_close_ports_() {
    core::Mutex::Lock lock(mutex_);

    while (core::SharedPtr<BasicPort> port = open_ports_.front()) {
        open_ports_.remove(*port);
        closing_ports_.push_back(*port);

        port->async_close();
    }
}

void Transceiver::close_sems_() {
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
    core::Mutex::Lock lock(mutex_);

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

        task->result = (this->*(task->fn))(*task);
        task->done = true;
    }

    cond_.broadcast();
}

bool Transceiver::add_udp_receiver_(Task& task) {
    core::SharedPtr<BasicPort> rp =
        new (allocator_) UDPReceiverPort(*this, *task.address, loop_, *task.writer,
                                         packet_pool_, buffer_pool_, allocator_);

    if (!rp) {
        roc_log(LogError, "transceiver: can't add port %s: can't allocate receiver",
                address::socket_addr_to_str(*task.address).c_str());

        return false;
    }

    task.port = rp.get();

    if (!rp->open()) {
        roc_log(LogError, "transceiver: can't add port %s: can't start receiver",
                address::socket_addr_to_str(*task.address).c_str());

        closing_ports_.push_back(*rp);
        rp->async_close();

        return false;
    }

    *task.address = rp->address();
    open_ports_.push_back(*rp);

    return true;
}

bool Transceiver::add_udp_sender_(Task& task) {
    core::SharedPtr<UDPSenderPort> sp =
        new (allocator_) UDPSenderPort(*this, *task.address, loop_, allocator_);
    if (!sp) {
        roc_log(LogError, "transceiver: can't add port %s: can't allocate sender",
                address::socket_addr_to_str(*task.address).c_str());

        return false;
    }

    task.port = sp.get();

    if (!sp->open()) {
        roc_log(LogError, "transceiver: can't add port %s: can't start sender",
                address::socket_addr_to_str(*task.address).c_str());

        closing_ports_.push_back(*sp);
        sp->async_close();

        return false;
    }

    task.writer = sp.get();
    *task.address = sp->address();

    open_ports_.push_back(*sp);

    return true;
}

bool Transceiver::remove_port_(Task& task) {
    roc_log(LogDebug, "transceiver: removing port %s",
            address::socket_addr_to_str(*task.address).c_str());

    core::SharedPtr<BasicPort> curr = open_ports_.front();
    while (curr) {
        core::SharedPtr<BasicPort> next = open_ports_.nextof(*curr);

        if (curr->address() == *task.address) {
            open_ports_.remove(*curr);
            closing_ports_.push_back(*curr);

            task.port = curr.get();
            curr->async_close();

            return true;
        }

        curr = next;
    }

    return false;
}

void Transceiver::wait_port_closed_(const BasicPort& port) {
    core::Mutex::Lock lock(mutex_);

    while (port_is_closing_(port)) {
        cond_.wait();
    }
}

bool Transceiver::port_is_closing_(const BasicPort& port) {
    for (core::SharedPtr<BasicPort> pp = closing_ports_.front(); pp;
         pp = closing_ports_.nextof(*pp)) {
        if (pp.get() == &port) {
            return true;
        }
    }

    return false;
}

} // namespace netio
} // namespace roc
