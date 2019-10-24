/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/tcp_conn.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace netio {

TCPConn::TCPConn(const address::SocketAddr& dst_addr,
                 const char* type_str,
                 uv_loop_t& event_loop,
                 ICloseHandler& close_handler,
                 core::IAllocator& allocator)
    : BasicPort(allocator)
    , loop_(event_loop)
    , write_sem_initialized_(false)
    , handle_initialized_(false)
    , close_handler_(close_handler)
    , conn_notifier_(NULL)
    , dst_addr_(dst_addr)
    , closed_(false)
    , stopped_(true)
    , connect_status_(Status_None)
    , type_str_(type_str)
    , cond_(mutex_) {
}

TCPConn::~TCPConn() {
    roc_panic_if(handle_initialized_);
    roc_panic_if(write_sem_initialized_);
}

const address::SocketAddr& TCPConn::address() const {
    return src_addr_;
}

bool TCPConn::open() {
    if (int err = uv_async_init(&loop_, &write_sem_, write_sem_cb_)) {
        roc_log(LogError, "tcp conn (%s): uv_async_init(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return false;
    }
    write_sem_.data = this;
    write_sem_initialized_ = true;

    if (int err = uv_tcp_init(&loop_, &handle_)) {
        roc_log(LogError, "tcp conn (%s): uv_tcp_init(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return false;
    }
    handle_.data = this;
    handle_initialized_ = true;

    connect_req_.data = this;

    stopped_ = false;

    return true;
}

void TCPConn::async_close() {
    core::Mutex::Lock lock(mutex_);

    stopped_ = true;

    if (!write_tasks_.size() && connect_status_ != Status_None) {
        close_();
    }
}

const address::SocketAddr& TCPConn::destination_address() const {
    return dst_addr_;
}

bool TCPConn::connected() const {
    core::Mutex::Lock lock(mutex_);

    return connect_status_ == Status_OK;
}

bool TCPConn::accept(uv_stream_t* stream, IConnNotifier& conn_notifier) {
    roc_panic_if_not(stream);
    roc_panic_if(conn_notifier_);

    if (int err = uv_accept(stream, (uv_stream_t*)&handle_)) {
        roc_log(LogError,
                "tcp conn (%s): can't accept connection: uv_tcp_accept(): [%s] %s",
                type_str_, uv_err_name(err), uv_strerror(err));

        return false;
    }

    int addrlen = (int)dst_addr_.slen();
    if (int err = uv_tcp_getpeername(&handle_, src_addr_.saddr(), &addrlen)) {
        roc_log(LogError,
                "tcp conn (%s): can't accept connection: uv_tcp_getpeername(): [%s] %s",
                type_str_, uv_err_name(err), uv_strerror(err));

        return false;
    }

    if (addrlen != (int)src_addr_.slen()) {
        roc_log(
            LogError,
            "tcp conn (%s): uv_tcp_getpeername(): unexpected len: got=%lu expected=%lu",
            type_str_, (unsigned long)addrlen, (unsigned long)src_addr_.slen());

        return false;
    }

    conn_notifier_ = &conn_notifier;
    set_connect_status_(Status_OK);

    return true;
}

bool TCPConn::connect(IConnNotifier& conn_notifier) {
    roc_panic_if(conn_notifier_);

    if (int err =
            uv_tcp_connect(&connect_req_, &handle_, dst_addr_.saddr(), connect_cb_)) {
        roc_log(LogError, "tcp conn (%s): uv_tcp_connect(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    int addrlen = (int)dst_addr_.slen();
    if (int err = uv_tcp_getsockname(&handle_, src_addr_.saddr(), &addrlen)) {
        roc_log(LogError, "tcp conn (%s): uv_tcp_getsockname(): [%s] %s", type_str_,
                uv_err_name(err), uv_strerror(err));
        return false;
    }
    if (addrlen != (int)src_addr_.slen()) {
        roc_log(LogError,
                "tcp conn (%s): uv_tcp_getsockname(): unexpected len: got=%lu "
                "expected = %lu",
                type_str_, (unsigned long)addrlen, (unsigned long)src_addr_.slen());
        return false;
    }

    conn_notifier_ = &conn_notifier;

    return true;
}

bool TCPConn::write(const char* data, size_t len) {
    roc_panic_if_not(data);

    {
        core::Mutex::Lock lock(mutex_);

        if (stopped_) {
            return true;
        }

        if (!add_write_task(data, len)) {
            return false;
        }
    }

    if (int err = uv_async_send(&write_sem_)) {
        roc_panic("tcp conn (%s): uv_async_send(): [%s] %s", type_str_, uv_err_name(err),
                  uv_strerror(err));
    }

    return true;
}

ssize_t TCPConn::read(char* buf, size_t len) {
    roc_panic_if_not(buf);

    return stream_.read(buf, len);
}

void TCPConn::close_cb_(uv_handle_t* handle) {
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TCPConn& self = *(TCPConn*)handle->data;

    if (handle == (uv_handle_t*)&self.handle_) {
        self.handle_initialized_ = false;
    } else {
        self.write_sem_initialized_ = false;
    }

    if (self.handle_initialized_ || self.write_sem_initialized_) {
        return;
    }

    roc_log(LogInfo, "tcp conn (%s): closed: src=%s dst=%s", self.type_str_,
            address::socket_addr_to_str(self.src_addr_).c_str(),
            address::socket_addr_to_str(self.dst_addr_).c_str());

    self.closed_ = true;
    self.close_handler_.handle_closed(self);
}

void TCPConn::connect_cb_(uv_connect_t* req, int status) {
    roc_panic_if_not(req);
    roc_panic_if_not(req->data);

    TCPConn& self = *(TCPConn*)req->data;

    ConnectStatus conn_status = (status < 0) ? Status_Error : Status_OK;

    if (conn_status == Status_OK) {
        roc_log(LogInfo, "tcp conn (%s): connected: src=%s dst=%s", self.type_str_,
                address::socket_addr_to_str(self.src_addr_).c_str(),
                address::socket_addr_to_str(self.dst_addr_).c_str());
    } else {
        roc_log(LogError, "tcp conn (%s): failed to connect: src=%s dst=%s: [%s] %s",
                self.type_str_, address::socket_addr_to_str(self.src_addr_).c_str(),
                address::socket_addr_to_str(self.dst_addr_).c_str(), uv_err_name(status),
                uv_strerror(status));
    }

    if (conn_status == Status_OK) {
        if (int err = uv_read_start((uv_stream_t*)&self.handle_, self.alloc_cb_,
                                    self.read_cb_)) {
            roc_log(LogError, "tcp conn (%s): uv_read_start(): [%s] %s", self.type_str_,
                    uv_err_name(err), uv_strerror(err));

            conn_status = Status_Error;
        }
    }

    self.set_connect_status_(conn_status);
    self.conn_notifier_->notify_connected(conn_status == Status_OK);

    core::Mutex::Lock lock(self.mutex_);

    if (self.stopped_ && !self.write_tasks_.size()) {
        self.close_();
    }
}

void TCPConn::write_sem_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TCPConn& self = *(TCPConn*)handle->data;

    self.process_write_tasks_();
}

void TCPConn::write_cb_(uv_write_t* req, int status) {
    roc_panic_if_not(req);
    roc_panic_if_not(req->data);

    TCPConn& self = *(TCPConn*)req->data;

    core::SharedPtr<WriteTask> tp = ROC_CONTAINER_OF(req, WriteTask, request);

    // one reference for incref() called from write_sem_cb_()
    // one reference for the shared pointer above
    roc_panic_if(tp->getref() < 2);

    // decrement reference counter incremented in write_sem_cb_()
    tp->decref();

    if (status < 0) {
        roc_log(LogError, "tcp conn (%s): failed to write: src=%s dst=%s: [%s] %s",
                self.type_str_, address::socket_addr_to_str(self.src_addr_).c_str(),
                address::socket_addr_to_str(self.dst_addr_).c_str(), uv_err_name(status),
                uv_strerror(status));
    }

    self.conn_notifier_->notify_writable(status == 0);

    core::Mutex::Lock lock(self.mutex_);

    if (self.stopped_ && !self.write_tasks_.size()) {
        self.close_();
    }
}

void TCPConn::alloc_cb_(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
    roc_panic_if_not(buf);
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TCPConn& self = *(TCPConn*)handle->data;

    core::SharedPtr<StreamBuffer> bp =
        new (self.allocator_) StreamBuffer(self.allocator_);
    if (!bp) {
        roc_log(LogError, "tcp conn (%s): can't allocate buffer", self.type_str_);

        buf->base = NULL;
        buf->len = 0;

        return;
    }

    if (!bp->resize(size)) {
        roc_log(LogError, "tcp conn (%s): can't resize allocated buffer", self.type_str_);

        buf->base = NULL;
        buf->len = 0;

        return;
    }

    self.stream_.append(bp);

    buf->len = size;
    buf->base = (char*)bp->data();
}

void TCPConn::read_cb_(uv_stream_t* stream, ssize_t nread, const uv_buf_t*) {
    roc_panic_if_not(stream);
    roc_panic_if_not(stream->data);

    TCPConn& self = *(TCPConn*)stream->data;

    if (nread < 0) {
        roc_log(LogError, "tcp conn (%s): network error: src=%s dst=%s nread=%ld",
                self.type_str_, address::socket_addr_to_str(self.src_addr_).c_str(),
                address::socket_addr_to_str(self.dst_addr_).c_str(), (long)nread);
        return;
    }

    if (nread == 0) {
        return;
    }

    self.conn_notifier_->notify_readable();
}

void TCPConn::close_() {
    if (closed_) {
        return; // handle_closed() was already called
    }

    if (!handle_initialized_) {
        closed_ = true;
        close_handler_.handle_closed(*this);

        return;
    }

    if (handle_initialized_ && !uv_is_closing((uv_handle_t*)&handle_)) {
        if (int err = uv_read_stop((uv_stream_t*)&handle_)) {
            roc_log(LogError, "tcp conn (%s): uv_read_stop(): [%s] %s", type_str_,
                    uv_err_name(err), uv_strerror(err));
        }

        roc_log(LogInfo, "tcp conn (%s): closing: src=%s dst=%s", type_str_,
                address::socket_addr_to_str(src_addr_).c_str(),
                address::socket_addr_to_str(dst_addr_).c_str());

        uv_close((uv_handle_t*)&handle_, close_cb_);
    }

    if (write_sem_initialized_ && !uv_is_closing((uv_handle_t*)&write_sem_)) {
        uv_close((uv_handle_t*)&write_sem_, close_cb_);
    }
}

void TCPConn::set_connect_status_(ConnectStatus status) {
    core::Mutex::Lock lock(mutex_);

    connect_status_ = status;
}

bool TCPConn::add_write_task(const char* data, size_t len) {
    core::SharedPtr<WriteTask> task = new (allocator_) WriteTask(allocator_);
    if (!task->buffer.resize(len)) {
        return false;
    }
    memcpy(task->buffer.data(), data, len);

    write_tasks_.push_back(*task);

    return true;
}

void TCPConn::process_write_tasks_() {
    core::Mutex::Lock lock(mutex_);

    while (core::SharedPtr<WriteTask> tp = write_tasks_.front()) {
        uv_buf_t buf;
        buf.base = tp->buffer.data();
        buf.len = tp->buffer.size();

        tp->request.data = this;

        if (int err =
                uv_write(&tp->request, (uv_stream_t*)&handle_, &buf, 1, write_cb_)) {
            roc_log(LogError, "tcp conn (%s): uv_write(): [%s] %s", type_str_,
                    uv_err_name(err), uv_strerror(err));

            continue;
        }

        write_tasks_.remove(*tp);

        // will be decremented in write_cb_()
        tp->incref();
    }
}

} // namespace netio
} // namespace roc
