/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/event_loop.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/helpers.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace netio {

EventLoop::Task::Task()
    : func_(NULL)
    , state_(Pending)
    , success_(false)
    , port_handle_(NULL)
    , port_writer_(NULL)
    , sender_config_(NULL)
    , receiver_config_(NULL) {
}

bool EventLoop::Task::success() const {
    return state_ == Finished && success_;
}

EventLoop::Tasks::AddUdpReceiverPort::AddUdpReceiverPort(UdpReceiverConfig& config,
                                                         packet::IWriter& writer) {
    func_ = &EventLoop::task_add_udp_receiver_;
    port_writer_ = &writer;
    receiver_config_ = &config;
}

EventLoop::PortHandle EventLoop::Tasks::AddUdpReceiverPort::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(port_handle_);
    return port_handle_;
}

EventLoop::Tasks::AddUdpSenderPort::AddUdpSenderPort(UdpSenderConfig& config) {
    func_ = &EventLoop::task_add_udp_sender_;
    sender_config_ = &config;
}

EventLoop::PortHandle EventLoop::Tasks::AddUdpSenderPort::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(port_handle_);
    return port_handle_;
}

packet::IWriter* EventLoop::Tasks::AddUdpSenderPort::get_writer() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(port_writer_);
    return port_writer_;
}

EventLoop::Tasks::RemovePort::RemovePort(PortHandle handle) {
    func_ = &EventLoop::task_remove_port_;
    if (!handle) {
        roc_panic("event loop: handle is null");
    }
    port_ = (BasicPort*)handle;
}

EventLoop::Tasks::ResolveEndpointAddress::ResolveEndpointAddress(
    const address::EndpointURI& endpoint_uri) {
    func_ = &EventLoop::task_resolve_endpoint_address_;
    resolve_req_.endpoint_uri = &endpoint_uri;
}

const address::SocketAddr& EventLoop::Tasks::ResolveEndpointAddress::get_address() const {
    return resolve_req_.resolved_address;
}

EventLoop::EventLoop(packet::PacketPool& packet_pool,
                     core::BufferPool<uint8_t>& buffer_pool,
                     core::IAllocator& allocator)
    : packet_pool_(packet_pool)
    , buffer_pool_(buffer_pool)
    , allocator_(allocator)
    , started_(false)
    , loop_initialized_(false)
    , stop_sem_initialized_(false)
    , task_sem_initialized_(false)
    , task_cond_(mutex_)
    , resolver_(*this, loop_) {
    if (int err = uv_loop_init(&loop_)) {
        roc_log(LogError, "event loop: uv_loop_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return;
    }
    loop_initialized_ = true;

    if (int err = uv_async_init(&loop_, &stop_sem_, stop_sem_cb_)) {
        roc_log(LogError, "event loop: uv_async_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return;
    }
    stop_sem_.data = this;
    stop_sem_initialized_ = true;

    if (int err = uv_async_init(&loop_, &task_sem_, task_sem_cb_)) {
        roc_log(LogError, "event loop: uv_async_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        return;
    }
    task_sem_.data = this;
    task_sem_initialized_ = true;

    started_ = Thread::start();
}

EventLoop::~EventLoop() {
    if (started_) {
        if (int err = uv_async_send(&stop_sem_)) {
            roc_panic("event loop: uv_async_send(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    } else {
        close_all_sems_();
    }

    if (loop_initialized_) {
        if (started_) {
            Thread::join();
        } else {
            // If the thread was never started we should manually run the loop to
            // wait all opened handles to be closed. Otherwise, uv_loop_close()
            // will fail with EBUSY.
            EventLoop::run(); // non-virtual call from dtor
        }

        if (int err = uv_loop_close(&loop_)) {
            roc_panic("event loop: uv_loop_close(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    }

    roc_panic_if(joinable());
    roc_panic_if(open_ports_.size());
    roc_panic_if(closing_ports_.size());
    roc_panic_if(task_sem_initialized_);
    roc_panic_if(stop_sem_initialized_);
}

bool EventLoop::valid() const {
    return started_;
}

size_t EventLoop::num_ports() const {
    core::Mutex::Lock lock(mutex_);

    return open_ports_.size();
}

bool EventLoop::enqueue_and_wait(Task& task) {
    core::Mutex::Lock lock(mutex_);

    if (!valid()) {
        roc_panic("event loop: can't use invalid loop");
    }

    pending_tasks_.push_back(task);

    if (int err = uv_async_send(&task_sem_)) {
        roc_panic("event loop: uv_async_send(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }

    while (task.state_ != Task::Finished) {
        task_cond_.wait();
    }

    return task.success_;
}

void EventLoop::handle_closed(BasicPort& port) {
    core::Mutex::Lock lock(mutex_);

    if (!closing_ports_.contains(port)) {
        return;
    }

    roc_log(LogDebug, "event loop: asynchronous close finished: port %s",
            address::socket_addr_to_str(port.address()).c_str());

    finish_closing_tasks_(port);
    closing_ports_.remove(port);

    task_cond_.broadcast();
}

void EventLoop::handle_resolved(ResolverRequest& req) {
    core::Mutex::Lock lock(mutex_);

    Task& task = *ROC_CONTAINER_OF(&req, Task, resolve_req_);

    task.success_ = req.success;
    task.state_ = Task::Finished;

    task_cond_.broadcast();
}

void EventLoop::run() {
    roc_log(LogDebug, "event loop: starting event loop");

    int err = uv_run(&loop_, UV_RUN_DEFAULT);
    if (err != 0) {
        roc_log(LogInfo, "event loop: uv_run() returned non-zero");
    }

    roc_log(LogDebug, "event loop: finishing event loop");
}

void EventLoop::task_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);

    EventLoop& self = *(EventLoop*)handle->data;
    self.process_pending_tasks_();
}

void EventLoop::stop_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);

    EventLoop& self = *(EventLoop*)handle->data;
    self.close_all_ports_();
    self.close_all_sems_();
    self.process_pending_tasks_();
}

void EventLoop::close_all_ports_() {
    core::Mutex::Lock lock(mutex_);

    while (core::SharedPtr<BasicPort> port = open_ports_.front()) {
        open_ports_.remove(*port);
        async_close_port_(*port);
    }
}

void EventLoop::close_all_sems_() {
    if (task_sem_initialized_) {
        uv_close((uv_handle_t*)&task_sem_, NULL);
        task_sem_initialized_ = false;
    }

    if (stop_sem_initialized_) {
        uv_close((uv_handle_t*)&stop_sem_, NULL);
        stop_sem_initialized_ = false;
    }
}

void EventLoop::process_pending_tasks_() {
    core::Mutex::Lock lock(mutex_);

    bool notify = false;

    while (Task* task = pending_tasks_.front()) {
        pending_tasks_.remove(*task);

        (this->*(task->func_))(*task);

        if (task->state_ == Task::Finished) {
            notify = true;
        }
    }

    if (notify) {
        task_cond_.broadcast();
    }
}

void EventLoop::task_add_udp_receiver_(Task& task) {
    core::SharedPtr<BasicPort> rp = new (allocator_)
        UdpReceiverPort(*task.receiver_config_, *task.port_writer_, *this, loop_,
                        packet_pool_, buffer_pool_, allocator_);
    if (!rp) {
        roc_log(LogError, "event loop: can't add port %s: can't allocate receiver",
                address::socket_addr_to_str(task.receiver_config_->bind_address).c_str());
        task.success_ = false;
        task.state_ = Task::Finished;
        return;
    }

    task.port_ = rp;

    if (!rp->open()) {
        roc_log(LogError, "event loop: can't add port %s: can't start receiver",
                address::socket_addr_to_str(task.receiver_config_->bind_address).c_str());
        task.success_ = false;
        if (!async_close_port_(*rp)) {
            task.state_ = Task::Finished;
        } else {
            closing_tasks_.push_back(task);
            task.state_ = Task::Closing;
        }
        return;
    }

    open_ports_.push_back(*rp);

    task.receiver_config_->bind_address = rp->address();
    task.port_handle_ = (PortHandle)rp.get();

    task.success_ = true;
    task.state_ = Task::Finished;
}

void EventLoop::task_add_udp_sender_(Task& task) {
    core::SharedPtr<UdpSenderPort> sp =
        new (allocator_) UdpSenderPort(*task.sender_config_, *this, loop_, allocator_);
    if (!sp) {
        roc_log(LogError, "event loop: can't add port %s: can't allocate sender",
                address::socket_addr_to_str(task.sender_config_->bind_address).c_str());
        task.success_ = false;
        task.state_ = Task::Finished;
        return;
    }

    task.port_ = sp;

    if (!sp->open()) {
        roc_log(LogError, "event loop: can't add port %s: can't start sender",
                address::socket_addr_to_str(task.sender_config_->bind_address).c_str());
        task.success_ = false;
        if (!async_close_port_(*sp)) {
            task.state_ = Task::Finished;
        } else {
            closing_tasks_.push_back(task);
            task.state_ = Task::Closing;
        }
        return;
    }

    open_ports_.push_back(*sp);

    task.sender_config_->bind_address = sp->address();
    task.port_handle_ = (PortHandle)sp.get();
    task.port_writer_ = sp.get();

    task.success_ = true;
    task.state_ = Task::Finished;
}

void EventLoop::task_remove_port_(Task& task) {
    roc_log(LogDebug, "event loop: removing port %s",
            address::socket_addr_to_str(task.port_->address()).c_str());

    open_ports_.remove(*task.port_);

    task.success_ = true;
    if (!async_close_port_(*task.port_)) {
        task.state_ = Task::Finished;
    } else {
        closing_tasks_.push_back(task);
        task.state_ = Task::Closing;
    }
}

void EventLoop::task_resolve_endpoint_address_(Task& task) {
    if (!resolver_.async_resolve(task.resolve_req_)) {
        task.success_ = task.resolve_req_.success;
        task.state_ = Task::Finished;
        return;
    }

    task.state_ = Task::Pending;
}

bool EventLoop::async_close_port_(BasicPort& port) {
    if (!port.async_close()) {
        return false;
    }

    closing_ports_.push_back(port);
    return true;
}

void EventLoop::finish_closing_tasks_(const BasicPort& port) {
    Task* task = closing_tasks_.front();

    while (task) {
        Task* next_task = closing_tasks_.nextof(*task);

        if (task->port_.get() == &port) {
            task->state_ = Task::Finished;
            closing_tasks_.remove(*task);
        }

        task = next_task;
    }
}

} // namespace netio
} // namespace roc
