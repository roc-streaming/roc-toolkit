/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/parser.h
//! @brief RTP packet parser.

#ifndef ROC_RTP_PARSER_H_
#define ROC_RTP_PARSER_H_

#include "roc_core/noncopyable.h"
#include "roc_packet/iparser.h"
#include "roc_rtp/encoding_map.h"

namespace roc {
namespace rtp {

//! RTP packet parser.
class Parser : public packet::IParser, public core::NonCopyable<> {
public:
    //! Initialization.
    //!
    //! @b Parameters
    //!  - @p encoding_map is used to get packet parameters by its
    //!    payload type
    //!  - if @p inner_parser is not NULL, it is used to parse the
    //!    packet payload
    Parser(packet::IParser* inner_parser,
           const EncodingMap& encoding_map,
           core::IArena& arena);

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Parse packet from buffer.
    virtual status::StatusCode parse(packet::Packet& packet,
                                     const core::Slice<uint8_t>& buffer);

private:
    const EncodingMap& encoding_map_;
    packet::IParser* inner_parser_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_PARSER_H_
