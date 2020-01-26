/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/basic_port.h
//! @brief Basic network port.

#ifndef ROC_NETIO_BASIC_PORT_H_
#define ROC_NETIO_BASIC_PORT_H_

#include "roc_address/socket_addr.h"
#include "roc_core/iallocator.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"

namespace roc {
namespace netio {

//! Basic port interface.
class BasicPort : public core::RefCnt<BasicPort>, public core::ListNode {
public:
    //! Initialize.
    explicit BasicPort(core::IAllocator&);

    //! Destroy.
    virtual ~BasicPort();

    //! Get bind address.
    virtual const address::SocketAddr& address() const = 0;

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
    //!  true if asynchronous close was initiated or false if
    //!  the port is already closed.
    virtual bool async_close() = 0;

private:
    friend class core::RefCnt<BasicPort>;

    void destroy();

    core::IAllocator& allocator_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_BASIC_PORT_H_
