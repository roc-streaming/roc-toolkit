/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/network_loop.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace netio {

NetworkLoop::Tasks::AddUdpPort::AddUdpPort(UdpConfig& config) {
    func_ = &NetworkLoop::task_add_udp_port_;
    config_ = &config;
}

NetworkLoop::PortHandle NetworkLoop::Tasks::AddUdpPort::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(port_handle_);
    return (PortHandle)port_handle_;
}

NetworkLoop::Tasks::StartUdpSend::StartUdpSend(PortHandle handle) {
    func_ = &NetworkLoop::task_start_udp_send_;
    if (!handle) {
        roc_panic("network loop: port handle is null");
    }
    port_ = (BasicPort*)handle;
}

packet::IWriter& NetworkLoop::Tasks::StartUdpSend::get_outbound_writer() const {
    roc_panic_if(!success());
    roc_panic_if(!outbound_writer_);
    return *outbound_writer_;
}

NetworkLoop::Tasks::StartUdpRecv::StartUdpRecv(PortHandle handle,
                                               packet::IWriter& inbound_writer) {
    func_ = &NetworkLoop::task_start_udp_recv_;
    if (!handle) {
        roc_panic("network loop: port handle is null");
    }
    port_ = (BasicPort*)handle;
    inbound_writer_ = &inbound_writer;
}

NetworkLoop::Tasks::AddTcpServerPort::AddTcpServerPort(TcpServerConfig& config,
                                                       IConnAcceptor& conn_acceptor) {
    func_ = &NetworkLoop::task_add_tcp_server_;
    config_ = &config;
    conn_acceptor_ = &conn_acceptor;
}

NetworkLoop::PortHandle NetworkLoop::Tasks::AddTcpServerPort::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(port_handle_);
    return (PortHandle)port_handle_;
}

NetworkLoop::Tasks::AddTcpClientPort::AddTcpClientPort(TcpClientConfig& config,
                                                       IConnHandler& conn_handler) {
    func_ = &NetworkLoop::task_add_tcp_client_;
    config_ = &config;
    conn_handler_ = &conn_handler;
}

NetworkLoop::PortHandle NetworkLoop::Tasks::AddTcpClientPort::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(port_handle_);
    return (PortHandle)port_handle_;
}

NetworkLoop::Tasks::RemovePort::RemovePort(PortHandle handle) {
    func_ = &NetworkLoop::task_remove_port_;
    if (!handle) {
        roc_panic("network loop: port handle is null");
    }
    port_ = (BasicPort*)handle;
}

NetworkLoop::Tasks::ResolveEndpointAddress::ResolveEndpointAddress(
    const address::NetworkUri& endpoint_uri) {
    func_ = &NetworkLoop::task_resolve_endpoint_address_;
    resolve_req_.endpoint_uri = &endpoint_uri;
}

const address::SocketAddr&
NetworkLoop::Tasks::ResolveEndpointAddress::get_address() const {
    return resolve_req_.resolved_address;
}

NetworkLoop::NetworkLoop(core::IPool& packet_pool,
                         core::IPool& buffer_pool,
                         core::IArena& arena)
    : packet_factory_(packet_pool, buffer_pool)
    , arena_(arena)
    , started_(false)
    , loop_initialized_(false)
    , stop_sem_initialized_(false)
    , task_sem_initialized_(false)
    , resolver_(*this, loop_)
    , num_open_ports_(0)
    , init_status_(status::NoStatus) {
    if (int err = uv_loop_init(&loop_)) {
        roc_log(LogError, "network loop: uv_loop_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        init_status_ = status::StatusErrNetwork;
        return;
    }
    loop_initialized_ = true;

    if (int err = uv_async_init(&loop_, &stop_sem_, stop_sem_cb_)) {
        roc_log(LogError, "network loop: uv_async_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        init_status_ = status::StatusErrNetwork;
        return;
    }
    stop_sem_.data = this;
    stop_sem_initialized_ = true;

    if (int err = uv_async_init(&loop_, &task_sem_, task_sem_cb_)) {
        roc_log(LogError, "network loop: uv_async_init(): [%s] %s", uv_err_name(err),
                uv_strerror(err));
        init_status_ = status::StatusErrNetwork;
        return;
    }
    task_sem_.data = this;
    task_sem_initialized_ = true;

    if (!(started_ = Thread::start())) {
        init_status_ = status::StatusErrThread;
        return;
    }

    init_status_ = status::StatusOK;
}

NetworkLoop::~NetworkLoop() {
    if (started_) {
        if (int err = uv_async_send(&stop_sem_)) {
            roc_panic("network loop: uv_async_send(): [%s] %s", uv_err_name(err),
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
            NetworkLoop::run(); // non-virtual call from dtor
        }

        if (int err = uv_loop_close(&loop_)) {
            roc_panic("network loop: uv_loop_close(): [%s] %s", uv_err_name(err),
                      uv_strerror(err));
        }
    }

    roc_panic_if(is_joinable());
    roc_panic_if(open_ports_.size());
    roc_panic_if(closing_ports_.size());
    roc_panic_if(task_sem_initialized_);
    roc_panic_if(stop_sem_initialized_);
}

status::StatusCode NetworkLoop::init_status() const {
    return init_status_;
}

size_t NetworkLoop::num_ports() const {
    return (size_t)num_open_ports_;
}

void NetworkLoop::schedule(NetworkTask& task, INetworkTaskCompleter& completer) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (task.state_ != NetworkTask::StateInitialized) {
        roc_panic("network loop: can't use the same task multiple times");
    }

    task.completer_ = &completer;
    task.state_ = NetworkTask::StatePending;

    pending_tasks_.push_back(task);

    if (int err = uv_async_send(&task_sem_)) {
        roc_panic("network loop: uv_async_send(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }
}

bool NetworkLoop::schedule_and_wait(NetworkTask& task) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (task.state_ != NetworkTask::StateInitialized) {
        roc_panic("network loop: can't use the same task multiple times");
    }

    if (!task.sem_) {
        task.sem_.reset(new (task.sem_) core::Semaphore);
    }

    task.completer_ = NULL;
    task.state_ = NetworkTask::StatePending;

    pending_tasks_.push_back(task);

    if (int err = uv_async_send(&task_sem_)) {
        roc_panic("network loop: uv_async_send(): [%s] %s", uv_err_name(err),
                  uv_strerror(err));
    }

    task.sem_->wait();

    return task.success_;
}

void NetworkLoop::handle_terminate_completed(IConn& conn, void* arg) {
    core::SharedPtr<TcpConnectionPort> port(static_cast<TcpConnectionPort*>(&conn));

    if (!closing_ports_.contains(*port)) {
        roc_panic("network loop: port is not in closing ports list: %s",
                  port->descriptor());
    }

    roc_log(LogDebug, "network loop: asynchronous terminate finished: port %s",
            port->descriptor());

    NetworkTask* task = (NetworkTask*)arg;

    if (async_close_port_(port, task) == AsyncOp_Started) {
        roc_log(LogDebug, "network loop: initiated asynchronous close: port %s",
                port->descriptor());
    } else {
        roc_log(LogDebug, "network loop: closed port: port %s", port->descriptor());

        finish_closing_port_(port, task);
    }
}

void NetworkLoop::handle_close_completed(BasicPort& port_ref, void* arg) {
    core::SharedPtr<BasicPort> port(static_cast<BasicPort*>(&port_ref));

    if (!closing_ports_.contains(*port)) {
        roc_panic("network loop: port is not in closing ports list: %s",
                  port->descriptor());
    }

    roc_log(LogDebug, "network loop: asynchronous close finished: port %s",
            port->descriptor());

    finish_closing_port_(port, (NetworkTask*)arg);
}

void NetworkLoop::handle_resolved(ResolverRequest& req) {
    Tasks::ResolveEndpointAddress& task =
        *ROC_CONTAINER_OF(&req, Tasks::ResolveEndpointAddress, resolve_req_);

    task.success_ = req.success;
    finish_task_(task);
}

void NetworkLoop::run() {
    roc_log(LogDebug, "network loop: starting event loop");

    int err = uv_run(&loop_, UV_RUN_DEFAULT);
    if (err != 0) {
        roc_log(LogInfo, "network loop: uv_run() returned non-zero");
    }

    roc_log(LogDebug, "network loop: finishing event loop");
}

void NetworkLoop::task_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);

    NetworkLoop& self = *(NetworkLoop*)handle->data;
    self.process_pending_tasks_();
}

void NetworkLoop::stop_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);

    NetworkLoop& self = *(NetworkLoop*)handle->data;
    self.close_all_ports_();
    self.close_all_sems_();
    self.process_pending_tasks_();
}

void NetworkLoop::process_pending_tasks_() {
    // Using try_pop_front_exclusive() makes this method lock-free and wait-free.
    // try_pop_front_exclusive() may return NULL if the queue is not empty, but
    // push_back() is currently in progress. In this case we can exit the loop
    // before processing all tasks, but schedule() always calls uv_async_send()
    // after push_back(), so we'll wake up soon and process the rest tasks.
    while (NetworkTask* task = pending_tasks_.try_pop_front_exclusive()) {
        (this->*(task->func_))(*task);

        if (task->state_ == NetworkTask::StateFinishing) {
            finish_task_(*task);
        }
    }
}

void NetworkLoop::finish_task_(NetworkTask& task) {
    INetworkTaskCompleter* completer = task.completer_;

    task.state_ = NetworkTask::StateFinished;

    if (completer) {
        completer->network_task_completed(task);
    } else {
        task.sem_->post();
    }
}

void NetworkLoop::async_terminate_conn_port_(
    const core::SharedPtr<TcpConnectionPort>& port, NetworkTask* task) {
    closing_ports_.push_back(*port);

    port->attach_terminate_handler(*this, task);
    port->async_terminate(Term_Failure);
}

AsyncOperationStatus
NetworkLoop::async_close_port_(const core::SharedPtr<BasicPort>& port,
                               NetworkTask* task) {
    const AsyncOperationStatus status = port->async_close(*this, task);

    if (status == AsyncOp_Started) {
        if (!closing_ports_.contains(*port)) {
            closing_ports_.push_back(*port);
        }
    }

    return status;
}

void NetworkLoop::finish_closing_port_(const core::SharedPtr<BasicPort>& port,
                                       NetworkTask* task) {
    closing_ports_.remove(*port);

    if (task) {
        finish_task_(*task);
    }
}

void NetworkLoop::update_num_ports_() {
    num_open_ports_ = (int)open_ports_.size();
}

void NetworkLoop::close_all_ports_() {
    while (core::SharedPtr<BasicPort> port = open_ports_.front()) {
        open_ports_.remove(*port);
        async_close_port_(port, NULL);
    }
    update_num_ports_();
}

void NetworkLoop::close_all_sems_() {
    if (task_sem_initialized_) {
        uv_close((uv_handle_t*)&task_sem_, NULL);
        task_sem_initialized_ = false;
    }

    if (stop_sem_initialized_) {
        uv_close((uv_handle_t*)&stop_sem_, NULL);
        stop_sem_initialized_ = false;
    }
}

void NetworkLoop::task_add_udp_port_(NetworkTask& base_task) {
    Tasks::AddUdpPort& task = (Tasks::AddUdpPort&)base_task;

    core::SharedPtr<UdpPort> port =
        new (arena_) UdpPort(*task.config_, loop_, packet_factory_, arena_);
    if (!port) {
        roc_log(LogError, "network loop: can't add udp port %s: allocate failed",
                address::socket_addr_to_str(task.config_->bind_address).c_str());
        task.success_ = false;
        task.state_ = NetworkTask::StateFinishing;
        return;
    }

    task.port_ = port;

    if (!port->open()) {
        roc_log(LogError, "network loop: can't add udp port %s: start failed",
                address::socket_addr_to_str(task.config_->bind_address).c_str());
        task.success_ = false;
        if (async_close_port_(port, &task) == AsyncOp_Started) {
            task.state_ = NetworkTask::StateClosingPort;
        } else {
            task.state_ = NetworkTask::StateFinishing;
        }
        return;
    }

    open_ports_.push_back(*port);
    update_num_ports_();

    task.config_->bind_address = port->bind_address();
    task.port_handle_ = port.get();

    task.success_ = true;
    task.state_ = NetworkTask::StateFinishing;
}

void NetworkLoop::task_start_udp_send_(NetworkTask& base_task) {
    Tasks::StartUdpSend& task = (Tasks::StartUdpSend&)base_task;

    roc_log(LogDebug, "network loop: starting sending packets on port %s",
            task.port_->descriptor());

    core::SharedPtr<UdpPort> port = (UdpPort*)task.port_.get();

    if (!(task.outbound_writer_ = port->start_send())) {
        roc_log(LogError, "network loop: can't start sending on port %s",
                task.port_->descriptor());
        task.success_ = false;
        task.state_ = NetworkTask::StateFinishing;
        return;
    }

    task.success_ = true;
    task.state_ = NetworkTask::StateFinishing;
}

void NetworkLoop::task_start_udp_recv_(NetworkTask& base_task) {
    Tasks::StartUdpRecv& task = (Tasks::StartUdpRecv&)base_task;

    roc_log(LogDebug, "network loop: starting receiving packets on port %s",
            task.port_->descriptor());

    core::SharedPtr<UdpPort> port = (UdpPort*)task.port_.get();

    if (!port->start_recv(*task.inbound_writer_)) {
        roc_log(LogError, "network loop: can't start receiving on port %s",
                task.port_->descriptor());
        task.success_ = false;
        task.state_ = NetworkTask::StateFinishing;
        return;
    }

    task.success_ = true;
    task.state_ = NetworkTask::StateFinishing;
}

void NetworkLoop::task_add_tcp_server_(NetworkTask& base_task) {
    Tasks::AddTcpServerPort& task = (Tasks::AddTcpServerPort&)base_task;

    core::SharedPtr<TcpServerPort> port =
        new (arena_) TcpServerPort(*task.config_, *task.conn_acceptor_, loop_, arena_);
    if (!port) {
        roc_log(LogError,
                "network loop: can't add tcp server port %s: can't allocate tcp server",
                address::socket_addr_to_str(task.config_->bind_address).c_str());
        task.success_ = false;
        task.state_ = NetworkTask::StateFinishing;
        return;
    }

    task.port_ = port;

    if (!port->open()) {
        roc_log(LogError,
                "network loop: can't add tcp server port %s: can't start tcp server",
                address::socket_addr_to_str(task.config_->bind_address).c_str());
        task.success_ = false;
        if (async_close_port_(port, &task) == AsyncOp_Started) {
            task.state_ = NetworkTask::StateClosingPort;
        } else {
            task.state_ = NetworkTask::StateFinishing;
        }
        return;
    }

    open_ports_.push_back(*port);
    update_num_ports_();

    task.config_->bind_address = port->bind_address();
    task.port_handle_ = port.get();

    task.success_ = true;
    task.state_ = NetworkTask::StateFinishing;
}

void NetworkLoop::task_add_tcp_client_(NetworkTask& base_task) {
    Tasks::AddTcpClientPort& task = (Tasks::AddTcpClientPort&)base_task;

    core::SharedPtr<TcpConnectionPort> port =
        new (arena_) TcpConnectionPort(TcpConn_Client, loop_, arena_);
    if (!port) {
        roc_log(LogError,
                "network loop: can't add tcp client port %s: can't allocate tcp client",
                address::socket_addr_to_str(task.config_->remote_address).c_str());
        task.success_ = false;
        task.state_ = NetworkTask::StateFinishing;
        return;
    }

    task.port_ = port;

    if (!port->open()) {
        roc_log(LogError,
                "network loop: can't add tcp client port %s: can't start tcp client",
                address::socket_addr_to_str(task.config_->remote_address).c_str());
        task.success_ = false;
        if (async_close_port_(port, &task) == AsyncOp_Started) {
            task.state_ = NetworkTask::StateClosingPort;
        } else {
            task.state_ = NetworkTask::StateFinishing;
        }
        return;
    }

    if (!port->connect(*task.config_)) {
        roc_log(LogError,
                "network loop: can't add tcp client port %s: can't start tcp client",
                address::socket_addr_to_str(task.config_->remote_address).c_str());
        task.success_ = false;
        task.state_ = NetworkTask::StateClosingPort;
        async_terminate_conn_port_(port, &task);
        return;
    }

    port->attach_connection_handler(*task.conn_handler_);

    open_ports_.push_back(*port);
    update_num_ports_();

    task.config_->local_address = port->local_address();
    task.port_handle_ = port.get();

    task.success_ = true;
    task.state_ = NetworkTask::StateFinishing;
}

void NetworkLoop::task_remove_port_(NetworkTask& base_task) {
    Tasks::RemovePort& task = (Tasks::RemovePort&)base_task;

    roc_log(LogDebug, "network loop: removing port %s", task.port_->descriptor());

    open_ports_.remove(*task.port_);
    update_num_ports_();

    task.success_ = true;
    if (async_close_port_(task.port_, &task) == AsyncOp_Started) {
        task.state_ = NetworkTask::StateClosingPort;
    } else {
        task.state_ = NetworkTask::StateFinishing;
    }
}

void NetworkLoop::task_resolve_endpoint_address_(NetworkTask& base_task) {
    Tasks::ResolveEndpointAddress& task = (Tasks::ResolveEndpointAddress&)base_task;

    if (!resolver_.async_resolve(task.resolve_req_)) {
        task.success_ = task.resolve_req_.success;
        task.state_ = NetworkTask::StateFinishing;
        return;
    }

    task.state_ = NetworkTask::StatePending;
}

} // namespace netio
} // namespace roc
