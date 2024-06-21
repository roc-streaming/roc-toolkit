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

//! Packet reading mode.
enum PacketReadMode {
    //! Read packet and remove in queue.
    //! @note
    //!  Next call to read will return new packet.
    ModeFetch,

    //! Read packet but keep it in queue.
    //! @note
    //!  Next call to read typically will return same packet.
    //!  However it may also return another packet if an older
    //!  packet arrives by the time of the next read.
    ModePeek
};

//! Packet reader interface.
class IReader {
public:
    virtual ~IReader();

    //! Read packet.
    //!
    //! @note
    //!  @p packet is output-only parameter, it is set to
    //!  the returned packet.
    //!
    //! @returns
    //!  - If packet was successfully read, returns status::StatusOK and sets
    //!    @p packet to the returned packet.
    //!  - If there are no errors but also no packets to read, returns
    //!    status::StatusDrain.
    //!  - Otherwise, returns an error.
    //!
    //! @see status::StatusCode.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(PacketPtr& packet,
                                                       PacketReadMode mode) = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IREADER_H_
