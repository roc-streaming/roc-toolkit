/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/ireader.h
//! @brief Packet reader interface.

#ifndef ROC_PACKET_IREADER_H_
#define ROC_PACKET_IREADER_H_

#include "roc_core/attributes.h"
#include "roc_packet/packet.h"
#include "roc_status/status_code.h"

namespace roc {
namespace packet {

//! Packet reader interface.
class IReader {
public:
    virtual ~IReader();

    //! Read packet.
    //!
    //! @returns
    //!  - If packet was successfully read, returns status::StatusOK and sets
    //!    @p packet to non-null.
    //!  - Otherwise, returns an error and sets @p packet to null.
    //!
    //! @see status::StatusCode.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(PacketPtr& packet) = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IREADER_H_
