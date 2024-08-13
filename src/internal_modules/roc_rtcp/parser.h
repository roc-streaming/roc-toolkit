/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/parser.h
//! @brief RTCP packet parser.

#ifndef ROC_RTCP_PARSER_H_
#define ROC_RTCP_PARSER_H_

#include "roc_core/noncopyable.h"
#include "roc_packet/iparser.h"

namespace roc {
namespace rtcp {

//! RTCP packet parser.
//!
//! @remarks
//!  Unlike other parsers, this one just records the buffer into RTCP part of the packet
//!  and doesn't inspect the packet itself. The actual parsing is done later in
//!  rtcp::Communicator using rtcp::Traverser.
class Parser : public packet::IParser, public core::NonCopyable<> {
public:
    //! Initialization.
    explicit Parser(core::IArena& arena);

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Parse packet from buffer.
    virtual bool parse(packet::Packet& packet, const core::Slice<uint8_t>& buffer);
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_PARSER_H_
