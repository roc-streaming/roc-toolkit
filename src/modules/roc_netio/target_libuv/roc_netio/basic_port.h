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
#include "roc_core/ref_counter.h"
#include "roc_core/string_builder.h"
#include "roc_netio/iclose_handler.h"
#include "roc_netio/operation_status.h"

namespace roc {
namespace netio {

//! Basic port interface.
class BasicPort : public core::RefCounter<BasicPort>, public core::ListNode {
public:
    //! Initialize.
    explicit BasicPort(core::IAllocator&);

    //! Destroy.
    virtual ~BasicPort();

    //! Get port name.
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
    //! Get memory allocator.
    core::IAllocator& allocator();

    //! Format descriptor into internal buffer.
    void update_descriptor();

    //! Format descriptor.
    virtual void format_descriptor(core::StringBuilder& b) = 0;

private:
    friend class core::RefCounter<BasicPort>;

    void destroy();

    core::IAllocator& allocator_;

    enum { MaxDescriptorLen = address::SocketAddr::MaxStrLen + 48 };

    char descriptor_[MaxDescriptorLen];
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_BASIC_PORT_H_
