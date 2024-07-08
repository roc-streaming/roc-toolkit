/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/iconn_handler.h
//! @brief Connection event handler interface.

#ifndef ROC_NETIO_ICONN_HANDLER_H_
#define ROC_NETIO_ICONN_HANDLER_H_

#include "roc_core/allocation_policy.h"
#include "roc_core/ref_counted.h"
#include "roc_netio/iconn.h"

namespace roc {
namespace netio {

//! Connection event handler interface.
//!
//! Workflow
//! --------
//!
//! - first, either connection_refused() or connection_established() is called
//!   excactly once for connection
//!
//! - these two calls are the point where the user can obtain IConn reference
//!   for the first time; the same reference will be then passed to other callbacks
//!
//! - after obtaining IConn reference, the user is responsible for terminating
//!   connection when it's no longer needed
//!
//! - connection_refused() call is possible only for client-side conenction
//!
//! - after connection is established, connection_writable() and
//!   connection_readable() are called repeatedly whenever it becomes
//!   possible to write or read data from connection
//!
//! - if an established connection fails asynchronously, it becomes readable
//!   and writable, and the next I/O operation will return error
//!
//! - after an asynchronous terminate is issued, no other callbacks
//!   except connection_terminated() are ever called
//!
//! - when an asynchronous terminate is completed, connection_terminated()
//!   is called; connection is still usable inside this callback
//!
//! - after connection_terminated() returns, the handler is never ever used for
//!   this connection, and the connection is destroyed
//!
//! - even after connection_terminated() call, the handler should not be
//!   destroyed until IConnAcceptor callback
//!
//! @note
//!  - Methods are called from the network loop thread.
//!  - Methods should not block.
class IConnHandler : public core::RefCounted<IConnHandler, core::NoopAllocation> {
public:
    virtual ~IConnHandler();

    //! Connection can't be established.
    virtual void connection_refused(IConn& conn) = 0;

    //! Connection successfully established.
    virtual void connection_established(IConn& conn) = 0;

    //! Connection becomes available for writing.
    virtual void connection_writable(IConn& conn) = 0;

    //! Connection becomes available for reading.
    virtual void connection_readable(IConn& conn) = 0;

    //! Connection is terminated and can't be accessed after this call.
    virtual void connection_terminated(IConn& conn) = 0;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_ICONN_HANDLER_H_
