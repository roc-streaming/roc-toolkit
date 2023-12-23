/*
 * Copyright (c) 2017 Roc Streaming authors
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
                                   const rtp::EncodingMap& encoding_map,
                                   core::IArena& arena)
    : core::RefCounted<ReceiverEndpoint, core::ArenaAllocation>(arena)
    , proto_(proto)
    , receiver_state_(receiver_state)
    , session_group_(session_group)
    , parser_(NULL)
    , valid_(false) {
    packet::IParser* parser = NULL;

    switch (proto) {
    case address::Proto_RTP:
    case address::Proto_RTP_LDPC_Source:
    case address::Proto_RTP_RS8M_Source:
        rtp_parser_.reset(new (rtp_parser_) rtp::Parser(encoding_map, NULL));
        if (!rtp_parser_) {
            return;
        }
        parser = rtp_parser_.get();
        break;
    default:
        break;
    }

    switch (proto) {
    case address::Proto_RTP_LDPC_Source:
        fec_parser_.reset(
            new (arena)
                fec::Parser<fec::LDPC_Source_PayloadID, fec::Source, fec::Footer>(parser),
            arena);
        if (!fec_parser_) {
            return;
        }
        parser = fec_parser_.get();
        break;
    case address::Proto_LDPC_Repair:
        fec_parser_.reset(
            new (arena)
                fec::Parser<fec::LDPC_Repair_PayloadID, fec::Repair, fec::Header>(parser),
            arena);
        if (!fec_parser_) {
            return;
        }
        parser = fec_parser_.get();
        break;
    case address::Proto_RTP_RS8M_Source:
        fec_parser_.reset(
            new (arena)
                fec::Parser<fec::RS8M_PayloadID, fec::Source, fec::Footer>(parser),
            arena);
        if (!fec_parser_) {
            return;
        }
        parser = fec_parser_.get();
        break;
    case address::Proto_RS8M_Repair:
        fec_parser_.reset(
            new (arena)
                fec::Parser<fec::RS8M_PayloadID, fec::Repair, fec::Header>(parser),
            arena);
        if (!fec_parser_) {
            return;
        }
        parser = fec_parser_.get();
        break;
    default:
        break;
    }

    switch (proto) {
    case address::Proto_RTCP:
        rtcp_parser_.reset(new (rtcp_parser_) rtcp::Parser());
        if (!rtcp_parser_) {
            return;
        }
        parser = rtcp_parser_.get();
        break;
    default:
        break;
    }

    if (!parser) {
        return;
    }

    parser_ = parser;
    valid_ = true;
}

bool ReceiverEndpoint::is_valid() const {
    return valid_;
}

address::Protocol ReceiverEndpoint::proto() const {
    roc_panic_if(!is_valid());

    return proto_;
}

packet::IWriter& ReceiverEndpoint::writer() {
    roc_panic_if(!is_valid());

    return *this;
}

status::StatusCode ReceiverEndpoint::pull_packets(core::nanoseconds_t current_time) {
    roc_panic_if(!is_valid());

    // Using try_pop_front_exclusive() makes this method lock-free and wait-free.
    // It may return NULL either if the queue is empty or if the packets in the
    // queue were added in a very short time or are being added currently. It's
    // acceptable to consider such packets late and to be pulled next time.
    while (packet::PacketPtr packet = queue_.try_pop_front_exclusive()) {
        if (!parser_->parse(*packet, packet->buffer())) {
            roc_log(LogDebug, "receiver endpoint: can't parse packet");
            continue;
        }

        const status::StatusCode code = session_group_.route_packet(packet, current_time);
        receiver_state_.add_pending_packets(-1);
        if (code != status::StatusOK) {
            return code;
        }
    }

    return status::StatusOK;
}

status::StatusCode ReceiverEndpoint::write(const packet::PacketPtr& packet) {
    roc_panic_if(!is_valid());

    if (!packet) {
        roc_panic("receiver endpoint: packet is null");
    }

    receiver_state_.add_pending_packets(+1);

    queue_.push_back(*packet);

    return status::StatusOK;
}

} // namespace pipeline
} // namespace roc
