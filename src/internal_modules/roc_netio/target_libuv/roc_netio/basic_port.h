/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/basic_port.h
//! @brief Base class for ports.

#ifndef ROC_NETIO_BASIC_PORT_H_
#define ROC_NETIO_BASIC_PORT_H_

#include "roc_address/socket_addr.h"
#include "roc_core/iarena.h"
#include "roc_core/list_node.h"
#include "roc_core/ref_counted.h"
#include "roc_core/string_builder.h"
#include "roc_netio/iclose_handler.h"
#include "roc_netio/operation_status.h"

namespace roc {
namespace netio {

//! Base class for ports.
//!
//! Port is a transport-level endpoint, sending or receiving data from remote
//! peer, like UDP sender or receiver, TCP listening socket, or TCP connection.
//!
//! The following rules must be followed:
//!
//!  - if you called open(), you're responsible for calling async_close(),
//!    even if open() failed
//!  - if async_close() returned AsyncOp_Completed, the port was closed
//!    immediately, and you can now destroy it
//!  - if async_close() returned AsyncOp_Started, you should wait until
//!    close handler callback is invoked before destroying port
class BasicPort : public core::RefCounted<BasicPort, core::ArenaAllocation>,
                  public core::ListNode<> {
public:
    //! Initialize.
    explicit BasicPort(core::IArena&);

    //! Destroy.
    virtual ~BasicPort();

    //! Get a human-readable port description.
    //!
    //! @note
    //!  Port descriptor may change during initial configuration.
    const char* descriptor() const;

    //! Open port.
    //!
    //! @remarks
    //!  Should be called from the event loop thread.
    virtual bool open() = 0;

    //! Asynchronous close.
    //!
    //! @remarks
    //!  Should be called from the event loop thread.
    //!
    //! @returns
    //!  status code indicating whether operation was completed immediately or
    //!  is scheduled for asynchronous execution
    virtual AsyncOperationStatus async_close(ICloseHandler& handler,
                                             void* handler_arg) = 0;

protected:
    //! Format descriptor and store into internal buffer.
    void update_descriptor();

    //! Implementation of descriptor formatting.
    virtual void format_descriptor(core::StringBuilder& b) = 0;

private:
    enum { MaxDescriptorLen = address::SocketAddr::MaxStrLen * 2 + 48 };

    char descriptor_[MaxDescriptorLen];
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_BASIC_PORT_H_
