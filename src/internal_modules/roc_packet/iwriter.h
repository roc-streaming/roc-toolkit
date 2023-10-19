/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/iwriter.h
//! @brief Packet writer interface.

#ifndef ROC_PACKET_IWRITER_H_
#define ROC_PACKET_IWRITER_H_

#include "roc_core/attributes.h"
#include "roc_packet/packet.h"
#include "roc_status/status_code.h"

namespace roc {
namespace packet {

//! Packet writer interface.
class IWriter {
public:
    virtual ~IWriter();

    //! Write packet.
    //!
    //! @returns
    //!  - If a returned code is not status::StatusOK, a packet is never written;
    //!  - If a packet is written, a returned code is always status::StatusOK.
    //!
    //! @see status::StatusCode.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const PacketPtr&) = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IWRITER_H_
