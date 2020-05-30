/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_endpoint.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_fec/composer.h"
#include "roc_fec/headers.h"
#include "roc_fec/parser.h"

namespace roc {
namespace pipeline {

ReceiverEndpoint::ReceiverEndpoint(address::Protocol proto,
                                   ReceiverState& receiver_state,
                                   ReceiverSessionGroup& session_group,
                                   const rtp::FormatMap& format_map,
                                   core::IAllocator& allocator)
    : proto_(proto)
    , allocator_(allocator)
    , receiver_state_(receiver_state)
    , session_group_(session_group)
    , parser_(NULL) {
    packet::IParser* parser = NULL;

    switch ((int)proto) {
    case address::Proto_RTP:
    case address::Proto_RTP_LDPC_Source:
    case address::Proto_RTP_RS8M_Source:
        rtp_parser_.reset(new (allocator) rtp::Parser(format_map, NULL), allocator);
        if (!rtp_parser_) {
            return;
        }
        parser = rtp_parser_.get();
        break;
    }

    switch ((int)proto) {
    case address::Proto_RTP_LDPC_Source:
        fec_parser_.reset(
            new (allocator)
                fec::Parser<fec::LDPC_Source_PayloadID, fec::Source, fec::Footer>(parser),
            allocator);
        if (!fec_parser_) {
            return;
        }
        parser = fec_parser_.get();
        break;
    case address::Proto_LDPC_Repair:
        fec_parser_.reset(
            new (allocator)
                fec::Parser<fec::LDPC_Repair_PayloadID, fec::Repair, fec::Header>(parser),
            allocator);
        if (!fec_parser_) {
            return;
        }
        parser = fec_parser_.get();
        break;
    case address::Proto_RTP_RS8M_Source:
        fec_parser_.reset(
            new (allocator)
                fec::Parser<fec::RS8M_PayloadID, fec::Source, fec::Footer>(parser),
            allocator);
        if (!fec_parser_) {
            return;
        }
        parser = fec_parser_.get();
        break;
    case address::Proto_RS8M_Repair:
        fec_parser_.reset(
            new (allocator)
                fec::Parser<fec::RS8M_PayloadID, fec::Repair, fec::Header>(parser),
            allocator);
        if (!fec_parser_) {
            return;
        }
        parser = fec_parser_.get();
        break;
    }

    parser_ = parser;
}

void ReceiverEndpoint::destroy() {
    allocator_.destroy(*this);
}

bool ReceiverEndpoint::valid() const {
    return parser_;
}

address::Protocol ReceiverEndpoint::proto() const {
    return proto_;
}

void ReceiverEndpoint::write(const packet::PacketPtr& packet) {
    roc_panic_if(!valid());
    roc_panic_if(!packet);

    receiver_state_.add_pending_packets(+1);

    queue_.push_back(*packet);
}

void ReceiverEndpoint::flush_packets() {
    roc_panic_if(!valid());

    // Using try_pop_front_exclusive() makes this method lock-free and wait-free.
    // It may return NULL either if the queue is empty or if the packets in the
    // queue were added in a very short time or are being added currently. It's
    // acceptable to consider such packets late for this flush and fetch next time.
    while (packet::PacketPtr packet = queue_.try_pop_front_exclusive()) {
        if (!parser_->parse(*packet, packet->data())) {
            roc_log(LogDebug, "receiver endpoint: can't parse packet");
            continue;
        }

        session_group_.route_packet(packet);

        receiver_state_.add_pending_packets(-1);
    }
}

} // namespace pipeline
} // namespace roc
