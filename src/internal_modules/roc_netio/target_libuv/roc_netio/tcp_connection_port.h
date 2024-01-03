/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/tcp_connection_port.h
//! @brief TCP connection.

#ifndef ROC_NETIO_TCP_CONNECTION_PORT_H_
#define ROC_NETIO_TCP_CONNECTION_PORT_H_

#include <uv.h>

#include "roc_address/socket_addr.h"
#include "roc_core/atomic.h"
#include "roc_core/mutex.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/seqlock.h"
#include "roc_core/shared_ptr.h"
#include "roc_netio/basic_port.h"
#include "roc_netio/iclose_handler.h"
#include "roc_netio/iconn.h"
#include "roc_netio/iconn_handler.h"
#include "roc_netio/iterminate_handler.h"
#include "roc_netio/socket_ops.h"

namespace roc {
namespace netio {

//! TCP connection parameters.
struct TcpConnectionConfig {
    //! Socket options.
    SocketOpts socket_options;
};

//! TCP connection parameters.
struct TcpClientConfig : TcpConnectionConfig {
    //! Local peer address to which we're bound.
    address::SocketAddr local_address;

    //! Remote peer address to which we're connected.
    address::SocketAddr remote_address;
};

//! TCP connection type.
enum TcpConnectionType {
    //! Local peer is client, remote peer is server.
    TcpConn_Client,

    //! Local peer is server, remote peer is client.
    TcpConn_Server
};

//! TCP connection port.
//!
//! Public interfaces
//! -----------------
//!
//! There are two important interfaces related to TCP connection:
//!  - IConn
//!  - IConnHandler
//!
//! IConn is implemented by TcpConnectionPort. The interface allows to retrieve
//! connection parameters and perform non-blocking I/O.
//!
//! IConnHandler is implemented by users of netio module. This interface is notified
//! about connection state changes (e.g. connection is established) and availability
//! of I/O (e.g. connection becomes readable).
//!
//! Thread access
//! -------------
//!
//! Methods that are not part of IConn interface are called from within other netio
//! classes, e.g. TcpServerPort, on the network loop thread.
//!
//! Methods from the IConn interface are called by users of netio module from any
//! thread. They are thread-safe and lock-free.
//!
//! Connection type and lifecycle
//! -----------------------------
//!
//! Connection can be client-side (connect call) or server-side (accept call).
//!
//! Client-side connection is created using AddTcpClientPort task of the network
//! loop, and is closed using RemovePort task. Before removing the port, the user
//! must call async_terminate() and wait until termination is completed.
//!
//! Server-side connection is created by TcpServerPort when it receives a new
//! incoming connection. To remove it, the user should call async_terminate().
//! When termination is completed, TcpServerPort automatically closes and
//! destroys connection.
//!
//! Connection workflow
//! -------------------
//!
//! The following rules must be followed:
//!
//!  - if you called open(), even if it failed, you're responsible for calling
//!    async_close() and waiting for its completion before destroying connection
//!  - after calling open(), you should call either accept() or connect() before
//!    using connection
//!  - if you called connect() or accept(), even if it failed, you're responsible
//!    for calling async_terminate() and waiting for its completion before calling
//!    async_close()
//!  - after connection is established and before it's terminated you can
//!    perform I/O
//!  - even if connection can't be established, async_terminate() still should be
//!    called before closing and destryoing connection
//!
//! Connection FSM
//! --------------
//!
//! TcpConnectionPort maintains an FSM and sees each operation or event handler as a
//! transition between states. Each operation is allowed only in certain states and
//! will panic when not used properly.
//!
//! State switch mostly happens on the network thread, however some limited set of
//! transitions is allowed from other threads. For this reason, state switching is
//! done using atomic operations.
class TcpConnectionPort : public BasicPort, public IConn {
public:
    //! Initialize.
    TcpConnectionPort(TcpConnectionType type, uv_loop_t& loop, core::IArena& arena);

    //! Destroy.
    virtual ~TcpConnectionPort();

    //! Open TCP connection.
    //! @remarks
    //!  Should be called from network loop thread.
    virtual bool open();

    //! Asynchronously close TCP connection.
    //! @remarks
    //!  Should be called from network loop thread.
    virtual AsyncOperationStatus async_close(ICloseHandler& handler, void* handler_arg);

    //! Establish conection by accepting it from listening socket.
    //! @remarks
    //!  Should be called from network loop thread.
    bool accept(const TcpConnectionConfig& config,
                const address::SocketAddr& server_address,
                SocketHandle server_socket);

    //! Establish connection to remote peer (asynchronously).
    //! @remarks
    //!  Should be called from network loop thread.
    bool connect(const TcpClientConfig& config);

    //! Set termination handler and start using it.
    //! @remarks
    //!  Should be called from network loop thread.
    void attach_terminate_handler(ITerminateHandler& handler, void* handler_arg);

    //! Set connection handler and start reporting events to it.
    //! @remarks
    //!  Should be called from network loop thread.
    void attach_connection_handler(IConnHandler& handler);

    //! Return address of the local peer.
    //! @remarks
    //!  Can be called from any thread.
    virtual const address::SocketAddr& local_address() const;

    //! Return address of the remote peer.
    //! @remarks
    //!  Can be called from any thread.
    virtual const address::SocketAddr& remote_address() const;

    //! Return true if there was a failure.
    //! @remarks
    //!  Can be called from any thread.
    virtual bool is_failed() const;

    //! Return true if the connection is writable.
    //! @remarks
    //!  Can be called from any thread.
    virtual bool is_writable() const;

    //! Return true if the connection is readable.
    //! @remarks
    //!  Can be called from any thread.
    virtual bool is_readable() const;

    //! Write @p buf of size @p len to the connection.
    //! @remarks
    //!  Can be called from any thread.
    virtual ssize_t try_write(const void* buf, size_t len);

    //! Read @p len bytes from the the connection to @p buf.
    //! @remarks
    //!  Can be called from any thread.
    virtual ssize_t try_read(void* buf, size_t len);

    //! Initiate asynchronous graceful shutdown.
    //! @remarks
    //!  Can be called from any thread.
    virtual void async_terminate(TerminationMode mode);

protected:
    //! Format descriptor.
    virtual void format_descriptor(core::StringBuilder& b);

private:
    // State of the connection FSM.
    enum ConnectionState {
        // not opened or already closed
        State_Closed,

        // open() is in progress
        State_Opening,

        // opened, waiting for connect() or accept()
        State_Opened,

        // accept() or connect() is in progress
        State_Connecting,

        // asynchronous connection failed, need terminate and close
        State_Refused,

        // asynchronous connection succeeded, do I/O and then terminate and close
        State_Established,

        // failure during I/O, need terminate and close
        State_Broken,

        // async_terminate() was called, asynchronous termination is in progress
        State_Terminating,

        // asynchronous termination completed, ready for closing
        State_Terminated,

        // async_close() was called, asynchronous close is in progress
        State_Closing
    };

    // Reading or writing status of the socket.
    enum IoStatus {
        // socket is not ready for I/O
        Io_NotAvailable,

        // socket is ready for reading or writing
        Io_Available,

        // read or write operation is in progress
        Io_InProgress
    };

    // I/O statistics.
    struct IoStats {
        // number of IConnHandler events
        core::Seqlock<uint64_t> rd_events;
        core::Seqlock<uint64_t> wr_events;

        // number of try_read() and try_write() calls
        uint64_t rd_calls;
        uint64_t wr_calls;

        // how much times SockErr_WouldBlock was returned
        uint64_t rd_wouldblock;
        uint64_t wr_wouldblock;

        // number of bytes transferred
        uint64_t rd_bytes;
        uint64_t wr_bytes;

        IoStats()
            : rd_events(0)
            , wr_events(0)
            , rd_calls(0)
            , wr_calls(0)
            , rd_wouldblock(0)
            , wr_wouldblock(0)
            , rd_bytes(0)
            , wr_bytes(0) {
        }
    };

    static const char* conn_state_to_str_(ConnectionState);

    static void poll_cb_(uv_poll_t* handle, int status, int events);
    static void start_terminate_cb_(uv_async_t* handle);
    static void finish_terminate_cb_(uv_handle_t* handle);
    static void close_cb_(uv_handle_t* handle);

    bool start_polling_();
    AsyncOperationStatus async_stop_polling_(uv_close_cb completion_cb);

    void disconnect_socket_();

    AsyncOperationStatus async_close_();

    void set_and_report_writable_();
    void set_and_report_readable_();

    ConnectionState get_state_() const;
    void switch_and_report_state_(ConnectionState new_state);
    bool maybe_switch_state_(ConnectionState expected_state,
                             ConnectionState desired_state);
    void report_state_(ConnectionState state);

    void set_conn_handler_(IConnHandler& handler);
    void unset_conn_handler_();

    void check_usable_(ConnectionState conn_state) const;
    void check_usable_for_io_(ConnectionState conn_state) const;

    void report_io_stats_();

    uv_loop_t& loop_;

    uv_poll_t poll_handle_;
    bool poll_handle_initialized_;
    bool poll_handle_started_;

    uv_async_t terminate_sem_;
    bool terminate_sem_initialized_;

    core::SharedPtr<IConnHandler> conn_handler_;

    ITerminateHandler* terminate_handler_;
    void* terminate_handler_arg_;

    ICloseHandler* close_handler_;
    void* close_handler_arg_;

    TcpConnectionType type_;

    address::SocketAddr local_address_;
    address::SocketAddr remote_address_;

    SocketHandle socket_;

    core::Atomic<int32_t> conn_state_;

    core::Atomic<int32_t> conn_was_established_;
    core::Atomic<int32_t> conn_was_failed_;

    core::Atomic<int32_t> writable_status_;
    core::Atomic<int32_t> readable_status_;

    bool got_stream_end_;

    core::Mutex io_mutex_;

    IoStats io_stats_;
    core::RateLimiter report_limiter_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TCP_CONNECTION_PORT_H_
