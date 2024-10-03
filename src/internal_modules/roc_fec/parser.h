/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/parser.h
//! @brief FECFRAME packet parser.

#ifndef ROC_FEC_PARSER_H_
#define ROC_FEC_PARSER_H_

#include "roc_core/log.h"
#include "roc_core/noncopyable.h"
#include "roc_fec/headers.h"
#include "roc_packet/iparser.h"

namespace roc {
namespace fec {

//! FECFRAME packet parser.
template <class PayloadID, PayloadID_Type Type, PayloadID_Pos Pos>
class Parser : public packet::IParser, public core::NonCopyable<> {
public:
    //! Initialization.
    //! @remarks
    //!  Parses FECFRAME header or footer and passes the rest to @p inner_parser
    //!  if it's not null.
    Parser(packet::IParser* inner_parser, core::IArena& arena)
        : IParser(arena)
        , inner_parser_(inner_parser) {
    }

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const {
        return status::StatusOK;
    }

    //! Parse packet from buffer.
    virtual status::StatusCode parse(packet::Packet& packet,
                                     const core::Slice<uint8_t>& buffer) {
        if (buffer.size() < sizeof(PayloadID)) {
            roc_log(LogDebug, "fec parser: bad packet, size < %d (payload id)",
                    (int)sizeof(PayloadID));
            return status::StatusBadBuffer;
        }

        const PayloadID* payload_id;
        if (Pos == Header) {
            payload_id = (const PayloadID*)buffer.data();
        } else {
            payload_id =
                (const PayloadID*)(buffer.data() + buffer.size() - sizeof(PayloadID));
        }

        if (Type == Repair) {
            packet.add_flags(packet::Packet::FlagRepair);
        }

        packet.add_flags(packet::Packet::FlagFEC);

        packet::FEC& fec = *packet.fec();

        fec.fec_scheme = PayloadID::fec_scheme();
        fec.encoding_symbol_id = payload_id->esi();
        fec.source_block_number = (packet::blknum_t)payload_id->sbn();
        fec.source_block_length = payload_id->k();
        fec.block_length = payload_id->n();

        if (Pos == Header) {
            fec.payload = buffer.subslice(sizeof(PayloadID), buffer.size());
        } else {
            fec.payload = buffer.subslice(0, buffer.size() - sizeof(PayloadID));
        }

        if (inner_parser_) {
            return inner_parser_->parse(packet, fec.payload);
        }

        return status::StatusOK;
    }

private:
    packet::IParser* inner_parser_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_PARSER_H_
