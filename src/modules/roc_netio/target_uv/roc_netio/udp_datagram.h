/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_uv/roc_netio/udp_datagram.h
//! @brief UDP datagram.

#ifndef ROC_NETIO_UDP_DATAGRAM_H_
#define ROC_NETIO_UDP_DATAGRAM_H_

#include <uv.h>

#include "roc_core/ipool.h"
#include "roc_core/shared_ptr.h"
#include "roc_datagram/idatagram.h"

namespace roc {
namespace netio {

//! UDP datagram.
class UDPDatagram : public datagram::IDatagram {
public:
    //! UDP datagram type.
    static const datagram::DatagramType Type;

    //! Get datagram containing request.
    static UDPDatagram* container_of(uv_udp_send_t*);

    //! Initialize empty datagram.
    UDPDatagram(core::IPool<UDPDatagram>&);

    //! Send request handle.
    uv_udp_send_t& request();

    //! Datagram type.
    virtual datagram::DatagramType type() const;

    //! Datagram payload.
    virtual const core::IByteBufferConstSlice& buffer() const;

    //! Set payload.
    virtual void set_buffer(const core::IByteBufferConstSlice&);

    //! Datagram sender address.
    virtual const datagram::Address& sender() const;

    //! Set sender address.
    virtual void set_sender(const datagram::Address&);

    //! Datagram receiver address.
    virtual const datagram::Address& receiver() const;

    //! Set receiver address.
    virtual void set_receiver(const datagram::Address&);

private:
    virtual void free();

    core::IByteBufferConstSlice buffer_;

    datagram::Address sender_;
    datagram::Address receiver_;

    uv_udp_send_t request_;

    core::IPool<UDPDatagram>& pool_;
};

//! UDP datagram smart pointer.
typedef core::SharedPtr<UDPDatagram> UDPDatagramPtr;

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_UDP_DATAGRAM_H_
