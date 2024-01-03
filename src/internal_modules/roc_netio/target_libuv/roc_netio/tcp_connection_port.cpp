/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/tcp_connection_port.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/scoped_lock.h"

namespace roc {
namespace netio {

namespace {

const core::nanoseconds_t StatsReportInterval = 60 * core::Second;

} // namespace

TcpConnectionPort::TcpConnectionPort(TcpConnectionType type,
                                     uv_loop_t& loop,
                                     core::IArena& arena)
    : BasicPort(arena)
    , loop_(loop)
    , poll_handle_initialized_(false)
    , poll_handle_started_(false)
    , terminate_sem_initialized_(false)
    , conn_handler_(NULL)
    , terminate_handler_(NULL)
    , terminate_handler_arg_(NULL)
    , close_handler_(NULL)
    , close_handler_arg_(NULL)
    , type_(type)
    , socket_(SocketInvalid)
    , conn_state_(State_Closed)
    , conn_was_established_(false)
    , conn_was_failed_(false)
    , writable_status_(Io_NotAvailable)
    , readable_status_(Io_NotAvailable)
    , got_stream_end_(false)
    , report_limiter_(StatsReportInterval) {
    BasicPort::update_descriptor();
}

TcpConnectionPort::~TcpConnectionPort() {
    const ConnectionState conn_state = get_state_();

    if (conn_state != State_Closed) {
        roc_panic("tcp conn: %s: unexpected connection state \"%s\" in destructor",
                  descriptor(), conn_state_to_str_(conn_state));
    }

    if (socket_ != SocketInvalid) {
        roc_panic("tcp conn: %s: socket was not closed before calling destructor",
                  descriptor());
    }

    if (poll_handle_initialized_ || terminate_sem_initialized_) {
        roc_panic("tcp conn: %s: some handles were not closed before calling destructor",
                  descriptor());
    }
}

bool TcpConnectionPort::open() {
    const ConnectionState conn_state = get_state_();

    if (conn_state != State_Closed) {
        roc_panic("tcp conn: %s: unexpected connection state \"%s\" in open()",
                  descriptor(), conn_state_to_str_(conn_state));
    }

    switch_and_report_state_(State_Opening);

    if (int err = uv_async_init(&loop_, &terminate_sem_, start_terminate_cb_)) {
        roc_log(LogError, "tcp conn: %s: uv_async_init(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }
    terminate_sem_.data = this;
    terminate_sem_initialized_ = true;

    switch_and_report_state_(State_Opened);

    return true;
}

AsyncOperationStatus TcpConnectionPort::async_close(ICloseHandler& handler,
                                                    void* handler_arg) {
    const ConnectionState conn_state = get_state_();

    if (conn_state != State_Opening && conn_state != State_Opened
        && conn_state != State_Terminated) {
        roc_panic("tcp conn: %s: unexpected connection state \"%s\" in async_close()",
                  descriptor(), conn_state_to_str_(conn_state));
    }

    if (close_handler_) {
        roc_panic("tcp conn: %s: can't call async_close() twice", descriptor());
    }

    close_handler_ = &handler;
    close_handler_arg_ = handler_arg;

    switch_and_report_state_(State_Closing);

    const AsyncOperationStatus status = async_close_();

    if (status == AsyncOp_Completed) {
        switch_and_report_state_(State_Closed);
    }

    return status;
}

bool TcpConnectionPort::accept(const TcpConnectionConfig& config,
                               const address::SocketAddr& server_address,
                               SocketHandle server_socket) {
    roc_panic_if_not(type_ == TcpConn_Server);

    const ConnectionState conn_state = get_state_();

    if (conn_state != State_Opened) {
        roc_panic("tcp conn: %s: unexpected connection state \"%s\" in accept()",
                  descriptor(), conn_state_to_str_(conn_state));
    }

    switch_and_report_state_(State_Connecting);

    local_address_ = server_address;

    if (!socket_accept(server_socket, socket_, remote_address_)) {
        roc_log(LogError, "tcp conn: %s: can't accept connection: socket_accept() failed",
                descriptor());
        return false;
    }

    if (!socket_setup(socket_, config.socket_options)) {
        roc_log(LogError, "tcp conn: %s: can't accept connection: socket_setup() failed",
                descriptor());
        return false;
    }

    if (!start_polling_()) {
        roc_log(LogError, "tcp conn: %s: can't accept connection: can't start polling",
                descriptor());
        return false;
    }

    update_descriptor();

    roc_log(LogDebug, "tcp conn: %s: accepted connection", descriptor());

    switch_and_report_state_(State_Established);

    return true;
}

bool TcpConnectionPort::connect(const TcpClientConfig& config) {
    roc_panic_if_not(type_ == TcpConn_Client);

    const ConnectionState conn_state = get_state_();

    if (conn_state != State_Opened) {
        roc_panic("tcp conn: %s: unexpected connection state \"%s\" in connect()",
                  descriptor(), conn_state_to_str_(conn_state));
    }

    switch_and_report_state_(State_Connecting);

    local_address_ = config.local_address;
    remote_address_ = config.remote_address;

    if (!socket_create(local_address_.family(), SocketType_Tcp, socket_)) {
        roc_log(LogError,
                "tcp conn: %s: can't connect to remote peer: socket_create() failed",
                descriptor());
        return false;
    }

    if (!socket_setup(socket_, config.socket_options)) {
        roc_log(LogError,
                "tcp conn: %s: can't connect to remote peer: socket_setup() failed",
                descriptor());
        return false;
    }

    if (!socket_bind(socket_, local_address_)) {
        roc_log(LogError,
                "tcp conn: %s: can't connect to remote peer: socket_bind() failed",
                descriptor());
        return false;
    }

    bool completed_immediately = false;

    if (!socket_begin_connect(socket_, remote_address_, completed_immediately)) {
        roc_log(
            LogError,
            "tcp conn: %s: can't connect to remote peer: socket_begin_connect() failed",
            descriptor());
        return false;
    }

    if (!start_polling_()) {
        roc_log(LogError,
                "tcp conn: %s: can't connect to remote peer: can't start polling",
                descriptor());
        return false;
    }

    update_descriptor();

    if (completed_immediately) {
        roc_log(LogDebug, "tcp conn: %s: completed connection immediately", descriptor());
        switch_and_report_state_(State_Established);
    } else {
        roc_log(LogDebug, "tcp conn: %s: initiated asynchronous connect", descriptor());
    }

    return true;
}

void TcpConnectionPort::attach_terminate_handler(ITerminateHandler& handler,
                                                 void* handler_arg) {
    const ConnectionState conn_state = get_state_();

    check_usable_(conn_state);

    if (terminate_handler_) {
        roc_panic("tcp conn: %s: already have terminate handler", descriptor());
    }

    terminate_handler_ = &handler;
    terminate_handler_arg_ = handler_arg;
}

void TcpConnectionPort::attach_connection_handler(IConnHandler& handler) {
    const ConnectionState conn_state = get_state_();

    check_usable_(conn_state);

    set_conn_handler_(handler);

    report_state_(conn_state);
}

const address::SocketAddr& TcpConnectionPort::local_address() const {
    const ConnectionState conn_state = get_state_();

    check_usable_(conn_state);

    return local_address_;
}

const address::SocketAddr& TcpConnectionPort::remote_address() const {
    const ConnectionState conn_state = get_state_();

    check_usable_(conn_state);

    return remote_address_;
}

bool TcpConnectionPort::is_failed() const {
    const ConnectionState conn_state = get_state_();

    check_usable_(conn_state);

    return conn_was_failed_;
}

bool TcpConnectionPort::is_writable() const {
    const ConnectionState conn_state = get_state_();

    check_usable_(conn_state);

    if (conn_state != State_Established && conn_state != State_Broken) {
        return false;
    }

    return writable_status_ != Io_NotAvailable;
}

bool TcpConnectionPort::is_readable() const {
    const ConnectionState conn_state = get_state_();

    check_usable_(conn_state);

    if (conn_state != State_Established && conn_state != State_Broken) {
        return false;
    }

    return readable_status_ != Io_NotAvailable;
}

ssize_t TcpConnectionPort::try_write(const void* buf, size_t len) {
    core::ScopedLock<core::Mutex> lock(io_mutex_);

    roc_panic_if_not(buf);

    const ConnectionState conn_state = get_state_();

    check_usable_for_io_(conn_state);

    if (conn_state != State_Established) {
        return SockErr_Failure;
    }

    writable_status_ = Io_InProgress;

    const ssize_t ret = socket_try_send(socket_, buf, len);

    writable_status_.compare_exchange(Io_InProgress,
                                      ret >= 0 ? Io_Available : Io_NotAvailable);

    if (ret < 0 && ret != SockErr_WouldBlock) {
        maybe_switch_state_(State_Established, State_Broken);
    }

    io_stats_.wr_calls++;
    if (ret > 0) {
        io_stats_.wr_bytes += (size_t)ret;
    } else if (ret == SockErr_WouldBlock) {
        io_stats_.wr_wouldblock++;
    }

    report_io_stats_();

    return ret;
}

ssize_t TcpConnectionPort::try_read(void* buf, size_t len) {
    core::ScopedLock<core::Mutex> lock(io_mutex_);

    roc_panic_if_not(buf);

    const ConnectionState conn_state = get_state_();

    check_usable_for_io_(conn_state);

    if (conn_state != State_Established) {
        return SockErr_Failure;
    }

    if (got_stream_end_) {
        return SockErr_StreamEnd;
    }

    readable_status_ = Io_InProgress;

    const ssize_t ret = socket_try_recv(socket_, buf, len);

    readable_status_.compare_exchange(Io_InProgress,
                                      ret >= 0 ? Io_Available : Io_NotAvailable);

    if (ret < 0 && ret != SockErr_WouldBlock) {
        if (ret == SockErr_StreamEnd) {
            got_stream_end_ = true;
        } else {
            maybe_switch_state_(State_Established, State_Broken);
        }
    }

    io_stats_.rd_calls++;
    if (ret > 0) {
        io_stats_.rd_bytes += (size_t)ret;
    } else if (ret == SockErr_WouldBlock) {
        io_stats_.rd_wouldblock++;
    }

    report_io_stats_();

    return ret;
}

void TcpConnectionPort::async_terminate(TerminationMode mode) {
    for (;;) {
        const ConnectionState conn_state = get_state_();

        if (conn_state == State_Terminating) {
            roc_panic("tcp conn: %s: can't call async_terminate() twice", descriptor());
        }

        check_usable_(conn_state);

        if (maybe_switch_state_(conn_state, State_Terminating)) {
            break;
        }
    }

    roc_log(LogDebug, "tcp conn: %s: initiating asynchronous terminate: mode=%s",
            descriptor(), termination_mode_to_str(mode));

    if (mode == Term_Failure) {
        conn_was_failed_ = true;
    }

    if (int err = uv_async_send(&terminate_sem_)) {
        roc_panic("tcp conn: %s: uv_async_send(): [%s] %s", descriptor(),
                  uv_err_name(err), uv_strerror(err));
    }
}

void TcpConnectionPort::poll_cb_(uv_poll_t* handle, int status, int events) {
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TcpConnectionPort& self = *(TcpConnectionPort*)handle->data;

    const ConnectionState conn_state = self.get_state_();

    if (conn_state == State_Connecting && status < 0) {
        roc_log(LogError,
                "tcp conn: %s: poll failed during asynchronous connect: [%s] %s",
                self.descriptor(), uv_err_name(status), uv_strerror(status));

        self.switch_and_report_state_(State_Refused);

        return;
    }

    if (conn_state == State_Connecting && (events & UV_WRITABLE)) {
        if (socket_end_connect(self.socket_)) {
            roc_log(LogDebug, "tcp conn: %s: asynchronous connect succeeded",
                    self.descriptor());

            self.switch_and_report_state_(State_Established);
        } else {
            roc_log(LogError, "tcp conn: %s: asynchronous connect failed",
                    self.descriptor());

            self.switch_and_report_state_(State_Refused);
        }

        return;
    }

    if (conn_state == State_Established && status < 0) {
        roc_log(LogError, "tcp conn: %s: poll failed: [%s] %s", self.descriptor(),
                uv_err_name(status), uv_strerror(status));

        self.switch_and_report_state_(State_Broken);

        self.set_and_report_readable_();
        self.set_and_report_writable_();

        return;
    }

    if (conn_state == State_Established && (events & (UV_WRITABLE | UV_READABLE))) {
        if (events & UV_WRITABLE) {
            self.set_and_report_writable_();
        }

        if (events & UV_READABLE) {
            self.set_and_report_readable_();
        }

        return;
    }

    roc_log(LogTrace, "tcp conn: %s: ignoring poll callback in state \"%s\"",
            self.descriptor(), conn_state_to_str_(conn_state));
}

void TcpConnectionPort::start_terminate_cb_(uv_async_t* handle) {
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TcpConnectionPort& self = *(TcpConnectionPort*)handle->data;

    roc_panic_if_not(self.get_state_() == State_Terminating);

    if (self.async_stop_polling_(finish_terminate_cb_) == AsyncOp_Completed) {
        finish_terminate_cb_((uv_handle_t*)handle);
    }
}

void TcpConnectionPort::finish_terminate_cb_(uv_handle_t* handle) {
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TcpConnectionPort& self = *(TcpConnectionPort*)handle->data;

    roc_panic_if_not(self.get_state_() == State_Terminating);

    self.poll_handle_initialized_ = false;

    self.disconnect_socket_();

    self.switch_and_report_state_(State_Terminated);
    self.unset_conn_handler_();

    if (self.terminate_handler_) {
        roc_log(LogDebug, "tcp conn: %s: invoking termination handler",
                self.descriptor());

        self.terminate_handler_->handle_terminate_completed(self,
                                                            self.terminate_handler_arg_);
    }
}

void TcpConnectionPort::close_cb_(uv_handle_t* handle) {
    roc_panic_if_not(handle);
    roc_panic_if_not(handle->data);

    TcpConnectionPort& self = *(TcpConnectionPort*)handle->data;

    roc_panic_if_not(self.get_state_() == State_Closing);

    if (handle == (uv_handle_t*)&self.terminate_sem_) {
        self.terminate_sem_initialized_ = false;
    }

    if (self.terminate_sem_initialized_) {
        return;
    }

    roc_log(LogDebug, "tcp conn: %s: closed connection", self.descriptor());

    self.switch_and_report_state_(State_Closed);

    if (self.close_handler_) {
        self.close_handler_->handle_close_completed(self, self.close_handler_arg_);
    }
}

bool TcpConnectionPort::start_polling_() {
    poll_handle_.data = this;

    if (int err = uv_poll_init_socket(&loop_, &poll_handle_, socket_)) {
        roc_log(LogError, "tcp conn: %s: uv_poll_init_socket(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    poll_handle_initialized_ = true;

    if (int err = uv_poll_start(&poll_handle_, UV_READABLE | UV_WRITABLE, poll_cb_)) {
        roc_log(LogError, "tcp conn: %s: uv_poll_start(): [%s] %s", descriptor(),
                uv_err_name(err), uv_strerror(err));
        return false;
    }

    poll_handle_started_ = true;

    return true;
}

AsyncOperationStatus TcpConnectionPort::async_stop_polling_(uv_close_cb completion_cb) {
    if (!poll_handle_initialized_) {
        return AsyncOp_Completed;
    }

    if (poll_handle_started_) {
        poll_handle_started_ = false;
        uv_poll_stop(&poll_handle_);
    }

    if (!uv_is_closing((uv_handle_t*)&poll_handle_)) {
        uv_close((uv_handle_t*)&poll_handle_, completion_cb);
    }

    return AsyncOp_Started;
}

void TcpConnectionPort::disconnect_socket_() {
    if (socket_ == SocketInvalid) {
        return;
    }

    if (conn_was_established_ && !conn_was_failed_) {
        roc_log(LogDebug, "tcp conn: %s: performing graceful shutdown", descriptor());

        if (!socket_shutdown(socket_)) {
            roc_log(LogError, "tcp conn: %s: shutdown failed", descriptor());
            conn_was_failed_ = true;
        }
    }

    if (conn_was_established_ && !conn_was_failed_) {
        roc_log(LogDebug, "tcp conn: %s: closing socket normally", descriptor());

        if (!socket_close(socket_)) {
            roc_log(LogError, "tcp conn: %s: close failed", descriptor());
            conn_was_failed_ = true;
        }
    } else {
        roc_log(LogDebug, "tcp conn: %s: closing socket with reset", descriptor());

        if (!socket_close_with_reset(socket_)) {
            roc_log(LogError, "tcp conn: %s: close failed", descriptor());
            conn_was_failed_ = true;
        }
    }

    socket_ = SocketInvalid;
}

AsyncOperationStatus TcpConnectionPort::async_close_() {
    if (!terminate_sem_initialized_) {
        roc_log(LogDebug, "tcp conn: %s: closed connection", descriptor());
        return AsyncOp_Completed;
    }

    roc_log(LogDebug, "tcp conn: %s: initiating asynchronous close", descriptor());

    if (terminate_sem_initialized_ && !uv_is_closing((uv_handle_t*)&terminate_sem_)) {
        uv_close((uv_handle_t*)&terminate_sem_, close_cb_);
    }

    return AsyncOp_Started;
}

void TcpConnectionPort::set_and_report_writable_() {
    io_stats_.wr_events.exclusive_store(io_stats_.wr_events.wait_load() + 1);

    writable_status_ = Io_Available;

    if (conn_handler_) {
        conn_handler_->connection_writable(*this);
    }
}

void TcpConnectionPort::set_and_report_readable_() {
    io_stats_.rd_events.exclusive_store(io_stats_.rd_events.wait_load() + 1);

    readable_status_ = Io_Available;

    if (conn_handler_) {
        conn_handler_->connection_readable(*this);
    }
}

TcpConnectionPort::ConnectionState TcpConnectionPort::get_state_() const {
    return (ConnectionState)(int)conn_state_;
}

void TcpConnectionPort::switch_and_report_state_(ConnectionState new_state) {
    if (new_state == State_Terminated) {
        // report before changing state to give the user a chance of
        // accessing connection in the termination callback
        report_state_(new_state);

        // switching to State_Terminated is possible only from State_Terminating
        if (!maybe_switch_state_(State_Terminating, new_state)) {
            roc_panic("tcp conn: %s: unexpected connection state \"%s\" when terminating",
                      descriptor(), conn_state_to_str_(get_state_()));
        }

        return;
    }

    for (;;) {
        const ConnectionState old_state = get_state_();

        if (old_state == new_state) {
            return;
        }

        if (maybe_switch_state_(old_state, new_state)) {
            break;
        }
    }

    // report after changing state
    report_state_(new_state);
}

bool TcpConnectionPort::maybe_switch_state_(ConnectionState expected_state,
                                            ConnectionState desired_state) {
    // set these flags even if we leave state unchanged
    if (desired_state == State_Established) {
        conn_was_established_ = true;
    } else if (desired_state == State_Refused || desired_state == State_Broken) {
        conn_was_failed_ = true;
    }

    // ignore all state changes after termination, except closing
    if ((expected_state == State_Terminating || expected_state == State_Terminated)
        && (expected_state != State_Terminating && desired_state != State_Terminated
            && desired_state != State_Closing)) {
        return true;
    }

    if (!conn_state_.compare_exchange(expected_state, desired_state)) {
        return false;
    }

    roc_log(LogDebug, "tcp conn: %s: switched connection state: \"%s\" -> \"%s\"",
            descriptor(), conn_state_to_str_(expected_state),
            conn_state_to_str_(desired_state));

    return true;
}

void TcpConnectionPort::report_state_(ConnectionState state) {
    if (!conn_handler_) {
        return;
    }

    switch (state) {
    case State_Refused:
        roc_log(LogTrace, "tcp conn: %s: invoking connection_refused() callback",
                descriptor());
        conn_handler_->connection_refused(*this);
        break;

    case State_Established:
        roc_log(LogTrace, "tcp conn: %s: invoking connection_established() callback",
                descriptor());
        conn_handler_->connection_established(*this);
        break;

    case State_Terminated:
        roc_log(LogTrace, "tcp conn: %s: invoking connection_terminated() callback",
                descriptor());
        conn_handler_->connection_terminated(*this);
        break;

    default:
        break;
    }
}

void TcpConnectionPort::set_conn_handler_(IConnHandler& handler) {
    if (conn_handler_ != NULL) {
        roc_panic("tcp conn: %s: already have handler", descriptor());
    }

    conn_handler_ = &handler;
}

void TcpConnectionPort::unset_conn_handler_() {
    if (conn_handler_) {
        conn_handler_ = NULL;
    }
}

void TcpConnectionPort::check_usable_(ConnectionState conn_state) const {
    switch (conn_state) {
    case State_Opening:
    case State_Opened:
        roc_panic("tcp conn: %s: attempt to use connection before accept() or connect()",
                  descriptor());

    case State_Connecting:
    case State_Refused:
    case State_Established:
    case State_Broken:
    case State_Terminating:
        return;

    case State_Terminated:
        roc_panic("tcp conn: %s: attempt to use connection after connection_terminated()",
                  descriptor());

    case State_Closing:
    case State_Closed:
        roc_panic("tcp conn: %s: attempt to use connection after async_close()",
                  descriptor());
    }
}

void TcpConnectionPort::check_usable_for_io_(ConnectionState conn_state) const {
    switch (conn_state) {
    case State_Refused:
    case State_Established:
    case State_Broken:
        return;

    default:
        roc_panic("tcp conn: %s: attempt to do io before connection_established() or "
                  "connection_refused(), or after async_terminate()",
                  descriptor());
    }
}

void TcpConnectionPort::report_io_stats_() {
    if (!report_limiter_.allow()) {
        return;
    }

    roc_log(
        LogDebug,
        "tcp conn: %s: (r/w) events=%lu/%lu calls=%lu/%lu wb=%lu/%lu bytes=%luK/%luK",
        descriptor(), (unsigned long)io_stats_.rd_events.wait_load(),
        (unsigned long)io_stats_.wr_events.wait_load(), (unsigned long)io_stats_.rd_calls,
        (unsigned long)io_stats_.wr_calls, (unsigned long)io_stats_.rd_wouldblock,
        (unsigned long)io_stats_.wr_wouldblock, (unsigned long)io_stats_.rd_bytes / 1024,
        (unsigned long)io_stats_.wr_bytes / 1024);
}

void TcpConnectionPort::format_descriptor(core::StringBuilder& b) {
    b.append_str("<tcpconn");

    if (type_ == TcpConn_Client) {
        b.append_str(" client");
    } else {
        b.append_str(" server");
    }

    b.append_str(" 0x");
    b.append_uint((unsigned long)this, 16);

    b.append_str(" local=");
    b.append_str(address::socket_addr_to_str(local_address_).c_str());

    b.append_str(" remote=");
    b.append_str(address::socket_addr_to_str(remote_address_).c_str());

    b.append_str(">");
}

const char* TcpConnectionPort::conn_state_to_str_(ConnectionState state) {
    switch (state) {
    case State_Closed:
        return "closed";

    case State_Opening:
        return "opening";

    case State_Opened:
        return "opened";

    case State_Connecting:
        return "connecting";

    case State_Refused:
        return "refused";

    case State_Established:
        return "established";

    case State_Broken:
        return "broken";

    case State_Terminating:
        return "terminating";

    case State_Terminated:
        return "terminated";

    case State_Closing:
        return "closing";
    }

    roc_panic("tcp conn: unknown state");
}

} // namespace netio
} // namespace roc
