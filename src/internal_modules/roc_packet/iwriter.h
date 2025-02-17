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
    //! @note
    //!  Writer is allowed to store shared pointer to the packet.
    //!  It can assume that the packet is read-only.
    //!
    //! @returns
    //!  - If packet was successfully and completely written, returns status::StatusOK,
    //!    otherwise, returns an error.
    //!  - In case of error, it's not guaranteed that pipeline state didn't change,
    //!    e.g. part of the packet may be written.
    //!
    //! @see status::StatusCode.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const PacketPtr&) = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IWRITER_H_
