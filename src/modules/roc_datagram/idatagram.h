/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_datagram/idatagram.h
//! @brief Datagram interface.

#ifndef ROC_DATAGRAM_IDATAGRAM_H_
#define ROC_DATAGRAM_IDATAGRAM_H_

#include "roc_core/byte_buffer.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"
#include "roc_core/shared_ptr.h"

#include "roc_datagram/address.h"

namespace roc {
namespace datagram {

//! Datagram type.
typedef const void* DatagramType;

//! Datagram interface.
class IDatagram : public core::RefCnt, public core::ListNode {
public:
    virtual ~IDatagram();

    //! Datagram type.
    //! @remarks
    //!  Each IDatagram implementation has its own unique type.
    virtual DatagramType type() const = 0;

    //! Datagram payload.
    virtual const core::IByteBufferConstSlice& buffer() const = 0;

    //! Set payload.
    virtual void set_buffer(const core::IByteBufferConstSlice&) = 0;

    //! Datagram sender address.
    virtual const Address& sender() const = 0;

    //! Set sender address.
    virtual void set_sender(const Address&) = 0;

    //! Datagram receiver address.
    virtual const Address& receiver() const = 0;

    //! Set receiver address.
    virtual void set_receiver(const Address&) = 0;
};

//! Datagram smart pointer.
typedef core::SharedPtr<IDatagram> IDatagramPtr;

} // namespace datagram
} // namespace roc

#endif // ROC_DATAGRAM_IDATAGRAM_H_
