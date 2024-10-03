/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_endpoint.h"
#include "roc_address/protocol.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_fec/composer.h"
#include "roc_fec/headers.h"
#include "roc_fec/parser.h"
#include "roc_pipeline/receiver_session_group.h"

namespace roc {
namespace pipeline {

ReceiverEndpoint::ReceiverEndpoint(address::Protocol proto,
                                   StateTracker& state_tracker,
                                   ReceiverSessionGroup& session_group,
                                   rtp::EncodingMap& encoding_map,
                                   const address::SocketAddr& inbound_address,
                                   packet::IWriter* outbound_writer,
                                   core::IArena& arena)
    : core::RefCounted<ReceiverEndpoint, core::ArenaAllocation>(arena)
    , proto_(proto)
    , state_tracker_(state_tracker)
    , session_group_(session_group)
    , composer_(NULL)
    , parser_(NULL)
    , inbound_address_(inbound_address)
    , init_status_(status::NoStatus) {
    packet::IComposer* composer = NULL;
    packet::IParser* parser = NULL;

    switch (proto) {
    case address::Proto_RTP:
    case address::Proto_RTP_LDPC_Source:
    case address::Proto_RTP_RS8M_Source:
        rtp_parser_.reset(new (rtp_parser_) rtp::Parser(NULL, encoding_map, arena));
        if ((init_status_ = rtp_parser_->init_status()) != status::StatusOK) {
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
            new (arena) fec::Parser<fec::LDPC_Source_PayloadID, fec::Source, fec::Footer>(
                parser, arena));
        if (!fec_parser_) {
            init_status_ = status::StatusNoMem;
            return;
        }
        if ((init_status_ = fec_parser_->init_status()) != status::StatusOK) {
            return;
        }
        parser = fec_parser_.get();
        break;
    case address::Proto_LDPC_Repair:
        fec_parser_.reset(
            new (arena) fec::Parser<fec::LDPC_Repair_PayloadID, fec::Repair, fec::Header>(
                parser, arena));
        if (!fec_parser_) {
            init_status_ = status::StatusNoMem;
            return;
        }
        if ((init_status_ = fec_parser_->init_status()) != status::StatusOK) {
            return;
        }
        parser = fec_parser_.get();
        break;
    case address::Proto_RTP_RS8M_Source:
        fec_parser_.reset(new (arena)
                              fec::Parser<fec::RS8M_PayloadID, fec::Source, fec::Footer>(
                                  parser, arena));
        if (!fec_parser_) {
            init_status_ = status::StatusNoMem;
            return;
        }
        if ((init_status_ = fec_parser_->init_status()) != status::StatusOK) {
            return;
        }
        parser = fec_parser_.get();
        break;
    case address::Proto_RS8M_Repair:
        fec_parser_.reset(new (arena)
                              fec::Parser<fec::RS8M_PayloadID, fec::Repair, fec::Header>(
                                  parser, arena));
        if (!fec_parser_) {
            init_status_ = status::StatusNoMem;
            return;
        }
        if ((init_status_ = fec_parser_->init_status()) != status::StatusOK) {
            return;
        }
        parser = fec_parser_.get();
        break;
    default:
        break;
    }

    switch (proto) {
    case address::Proto_RTCP:
        rtcp_composer_.reset(new (rtcp_composer_) rtcp::Composer(arena));
        if ((init_status_ = rtcp_composer_->init_status()) != status::StatusOK) {
            return;
        }
        composer = rtcp_composer_.get();

        rtcp_parser_.reset(new (rtcp_parser_) rtcp::Parser(arena));
        if ((init_status_ = rtcp_parser_->init_status()) != status::StatusOK) {
            return;
        }
        parser = rtcp_parser_.get();

        break;
    default:
        break;
    }

    // For receiver, parser is mandatory (inbound packets),
    // composer is optional (outbound packets).
    if (!parser) {
        roc_log(LogError, "receiver endpoint: unsupported protocol %s",
                address::proto_to_str(proto));
        init_status_ = status::StatusBadProtocol;
        return;
    }

    if (composer) {
        if (!outbound_writer) {
            roc_panic("receiver endpoint:"
                      " outbound writer is required by protocol %s, but is missing",
                      address::proto_to_str(proto));
        }

        // We don't pass outbound address to shipper, because packets produced by
        // rtcp::Communicator will already have non-empty destination address.
        // On receiver, we enable report_back mode, which tells Communicator to
        // collect addresses of all discovered senders and generate RTCP packets for
        // each of them, instead of sending all RTCP packets to a single address.
        shipper_.reset(new (shipper_) packet::Shipper(*composer, *outbound_writer, NULL));
        if ((init_status_ = shipper_->init_status()) != status::StatusOK) {
            return;
        }
    }

    composer_ = composer;
    parser_ = parser;

    init_status_ = status::StatusOK;
}

status::StatusCode ReceiverEndpoint::init_status() const {
    return init_status_;
}

address::Protocol ReceiverEndpoint::proto() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return proto_;
}

packet::IComposer* ReceiverEndpoint::outbound_composer() {
    roc_panic_if(init_status_ != status::StatusOK);

    return composer_;
}

packet::IWriter* ReceiverEndpoint::outbound_writer() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!composer_) {
        // Outbound packets are not supported.
        return NULL;
    }

    roc_panic_if(!shipper_);
    return shipper_.get();
}

const address::SocketAddr& ReceiverEndpoint::inbound_address() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return inbound_address_;
}

packet::IWriter& ReceiverEndpoint::inbound_writer() {
    roc_panic_if(init_status_ != status::StatusOK);

    return *this;
}

status::StatusCode ReceiverEndpoint::pull_packets(core::nanoseconds_t current_time) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(!parser_);

    // Using try_pop_front_exclusive() makes this method lock-free and wait-free.
    // It may return NULL either if the queue is empty or if the packets in the
    // queue were added in a very short time or are being added currently. It's
    // acceptable to consider such packets late and pull them next time.
    while (packet::PacketPtr packet = inbound_queue_.try_pop_front_exclusive()) {
        const status::StatusCode code = handle_packet_(packet, current_time);
        state_tracker_.unregister_packet();

        if (code != status::StatusOK) {
            return code;
        }
    }

    return status::StatusOK;
}

status::StatusCode ReceiverEndpoint::handle_packet_(const packet::PacketPtr& packet,
                                                    core::nanoseconds_t current_time) {
    if (parser_->parse(*packet, packet->buffer()) != status::StatusOK) {
        roc_log(LogDebug, "receiver endpoint: dropping bad packet: can't parse");
        return status::StatusOK;
    }

    const status::StatusCode code = session_group_.route_packet(packet, current_time);

    if (code == status::StatusNoRoute) {
        roc_log(LogDebug, "receiver endpoint: dropping bad packet: can't route");
        return status::StatusOK;
    }

    if (code != status::StatusOK) {
        roc_log(LogError, "receiver endpoint: error when handling packet: status=%s",
                status::code_to_str(code));
        return code;
    }

    return status::StatusOK;
}

// Implementation of inbound_writer().write()
status::StatusCode ReceiverEndpoint::write(const packet::PacketPtr& packet) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(!packet);
    roc_panic_if(!parser_);

    state_tracker_.register_packet();
    inbound_queue_.push_back(*packet);

    return status::StatusOK;
}

} // namespace pipeline
} // namespace roc
