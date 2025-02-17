/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/shipper.h
//! @brief Prepare and ship outgoing packets.

#ifndef ROC_PACKET_SHIPPER_H_
#define ROC_PACKET_SHIPPER_H_

#include "roc_address/socket_addr.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"

namespace roc {
namespace packet {

//! Prepare a packet for being sent.
class Shipper : public IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    //!  - @p composer - used to complete composing packets
    //!  - @p outbound_writer - destination writer
    //!  - @p outbound_address - destination address is assigned to packets, may be null
    Shipper(IComposer& composer,
            IWriter& outbound_writer,
            const address::SocketAddr* outbound_address);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Get destination address for outbound packets.
    const address::SocketAddr& outbound_address() const;

    //! Write outgoing packet.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const PacketPtr& packet);

private:
    IComposer& composer_;
    IWriter& outbound_writer_;
    address::SocketAddr outbound_address_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_SHIPPER_H_
