/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/iconn.h
//! @brief Connection interface.

#ifndef ROC_NETIO_ICONN_H_
#define ROC_NETIO_ICONN_H_

#include "roc_address/socket_addr.h"
#include "roc_core/stddefs.h"
#include "roc_netio/socket_ops.h"
#include "roc_netio/termination_mode.h"

namespace roc {
namespace netio {

//! Connection interface.
//!
//! All methods are thread-safe and non-blocking.
//!
//! All methods are also lock-free if there is no more than one simultaneous
//! writer or reader. IConn operations are never blocked by network thread
//! itself, but concurrent simultaneous writes and reads block each other.
class IConn {
public:
    virtual ~IConn();

    //! Return address of the local peer.
    virtual const address::SocketAddr& local_address() const = 0;

    //! Return address of the remote peer.
    virtual const address::SocketAddr& remote_address() const = 0;

    //! Return true if there was a failure.
    virtual bool is_failed() const = 0;

    //! Return true if the connection is writable.
    virtual bool is_writable() const = 0;

    //! Return true if the connection is readable.
    virtual bool is_readable() const = 0;

    //! Try writing @p buf of size @p len to the connection without blocking.
    //! @remarks
    //!  - @p buf should not be NULL.
    //!  - @p buf should have size at least of @p len bytes.
    //! @returns
    //!  number of bytes written (>= 0) or SocketError (< 0);
    virtual ssize_t try_write(const void* buf, size_t len) = 0;

    //! Try reading @p len bytes from the the connection to @p buf without blocking.
    //! @remarks
    //!  - @p buf should not be NULL.
    //!  - @p buf should have size at least of @p len bytes.
    //! @returns
    //!  number of bytes read (>= 0) or SocketError (< 0);
    virtual ssize_t try_read(void* buf, size_t len) = 0;

    //! Initiate asynchronous connection termination.
    //! @remarks
    //!  When termination is complete, IConnHandler::connection_terminated()
    //!  is called, and then connection object is destroyed.
    virtual void async_terminate(TerminationMode mode) = 0;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_ICONN_H_
