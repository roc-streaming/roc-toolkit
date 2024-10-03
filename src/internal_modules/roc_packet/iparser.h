/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/iparser.h
//! @brief Packet parser interface.

#ifndef ROC_PACKET_IPARSER_H_
#define ROC_PACKET_IPARSER_H_

#include "roc_core/allocation_policy.h"
#include "roc_core/slice.h"
#include "roc_packet/packet.h"
#include "roc_status/status_code.h"

namespace roc {
namespace packet {

//! Packet parser interface.
class IParser : public core::ArenaAllocation {
public:
    //! Initialize.
    explicit IParser(core::IArena& arena);

    //! Deinitialize.
    virtual ~IParser();

    //! Check if the object was successfully constructed.
    //! @returns
    //!  status::StatusOK if parser was initialized correctly,
    //!  or error code otherwise.
    virtual status::StatusCode init_status() const = 0;

    //! Parse packet from buffer.
    //! @remarks
    //!  Parses input @p buffer and fills @p packet. If the packet payload contains
    //!  an inner packet, calls the inner parser as well.
    //! @returns
    //!  status::StatusOK if the packet was successfully parsed,
    //!  or error code otherwise.
    virtual status::StatusCode parse(Packet& packet,
                                     const core::Slice<uint8_t>& buffer) = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IPARSER_H_
