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
    //!  - @p dest_address - destination address that is assigned to the outgoing packets.
    //!  - @p composer to compose a packet if necessary.
    //!  - @p writer to write outgoing packet.
    Shipper(const address::SocketAddr& dest_address,
            IComposer& composer,
            IWriter& writer);

    //! Write outgoing packet.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr& packet);

private:
    const address::SocketAddr dest_address_;

    IComposer& composer_;
    IWriter& writer_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_SHIPPER_H_
