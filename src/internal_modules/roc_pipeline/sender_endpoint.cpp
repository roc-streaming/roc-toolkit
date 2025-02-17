/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_endpoint.h"
#include "roc_address/protocol.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_fec/composer.h"
#include "roc_fec/headers.h"
#include "roc_pipeline/sender_session.h"
#include "roc_rtcp/parser.h"

namespace roc {
namespace pipeline {

SenderEndpoint::SenderEndpoint(address::Protocol proto,
                               StateTracker& state_tracker,
                               SenderSession& sender_session,
                               const address::SocketAddr& outbound_address,
                               packet::IWriter& outbound_writer,
                               core::IArena& arena)
    : proto_(proto)
    , state_tracker_(state_tracker)
    , sender_session_(sender_session)
    , composer_(NULL)
    , parser_(NULL)
    , init_status_(status::NoStatus) {
    packet::IComposer* composer = NULL;
    packet::IParser* parser = NULL;

    switch (proto) {
    case address::Proto_RTP:
    case address::Proto_RTP_LDPC_Source:
    case address::Proto_RTP_RS8M_Source:
        rtp_composer_.reset(new (rtp_composer_) rtp::Composer(NULL, arena));
        if ((init_status_ = rtp_composer_->init_status()) != status::StatusOK) {
            return;
        }
        composer = rtp_composer_.get();
        break;
    default:
        break;
    }

    switch (proto) {
    case address::Proto_RTP_LDPC_Source:
        fec_composer_.reset(
            new (arena)
                fec::Composer<fec::LDPC_Source_PayloadID, fec::Source, fec::Footer>(
                    composer, arena));
        if (!fec_composer_) {
            init_status_ = status::StatusNoMem;
            return;
        }
        if ((init_status_ = fec_composer_->init_status()) != status::StatusOK) {
            return;
        }
        composer = fec_composer_.get();
        break;
    case address::Proto_LDPC_Repair:
        fec_composer_.reset(
            new (arena)
                fec::Composer<fec::LDPC_Repair_PayloadID, fec::Repair, fec::Header>(
                    composer, arena));
        if (!fec_composer_) {
            init_status_ = status::StatusNoMem;
            return;
        }
        if ((init_status_ = fec_composer_->init_status()) != status::StatusOK) {
            return;
        }
        composer = fec_composer_.get();
        break;
    case address::Proto_RTP_RS8M_Source:
        fec_composer_.reset(
            new (arena) fec::Composer<fec::RS8M_PayloadID, fec::Source, fec::Footer>(
                composer, arena));
        if (!fec_composer_) {
            init_status_ = status::StatusNoMem;
            return;
        }
        if ((init_status_ = fec_composer_->init_status()) != status::StatusOK) {
            return;
        }
        composer = fec_composer_.get();
        break;
    case address::Proto_RS8M_Repair:
        fec_composer_.reset(
            new (arena) fec::Composer<fec::RS8M_PayloadID, fec::Repair, fec::Header>(
                composer, arena));
        if (!fec_composer_) {
            init_status_ = status::StatusNoMem;
            return;
        }
        if ((init_status_ = fec_composer_->init_status()) != status::StatusOK) {
            return;
        }
        composer = fec_composer_.get();
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

    // For sender, composer is mandatory (outbound packets),
    // parser is optional (inbound packets).
    if (!composer) {
        roc_log(LogError, "sender endpoint: unsupported protocol %s",
                address::proto_to_str(proto));
        init_status_ = status::StatusBadProtocol;
        return;
    }

    shipper_.reset(new (shipper_)
                       packet::Shipper(*composer, outbound_writer, &outbound_address));
    if ((init_status_ = shipper_->init_status()) != status::StatusOK) {
        return;
    }

    composer_ = composer;
    parser_ = parser;

    init_status_ = status::StatusOK;
}

status::StatusCode SenderEndpoint::init_status() const {
    return init_status_;
}

address::Protocol SenderEndpoint::proto() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return proto_;
}

const address::SocketAddr& SenderEndpoint::outbound_address() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return shipper_->outbound_address();
}

packet::IComposer& SenderEndpoint::outbound_composer() {
    roc_panic_if(init_status_ != status::StatusOK);

    return *composer_;
}

packet::IWriter& SenderEndpoint::outbound_writer() {
    roc_panic_if(init_status_ != status::StatusOK);

    return *shipper_;
}

packet::IWriter* SenderEndpoint::inbound_writer() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!parser_) {
        // Inbound packets are not supported.
        return NULL;
    }

    return this;
}

status::StatusCode SenderEndpoint::pull_packets(core::nanoseconds_t current_time) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!parser_) {
        // No inbound packets expected for this endpoint, only outbound.
        return status::StatusOK;
    }

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

status::StatusCode SenderEndpoint::handle_packet_(const packet::PacketPtr& packet,
                                                  core::nanoseconds_t current_time) {
    if (!parser_->parse(*packet, packet->buffer())) {
        roc_log(LogDebug, "sender endpoint: dropping bad packet: can't parse");
        return status::StatusOK;
    }

    const status::StatusCode code = sender_session_.route_packet(packet, current_time);

    if (code == status::StatusNoRoute) {
        roc_log(LogDebug, "sender endpoint: dropping bad packet: can't route");
        return status::StatusOK;
    }

    if (code != status::StatusOK) {
        roc_log(LogError, "sender endpoint: error when handling packet: status=%s",
                status::code_to_str(code));
        return code;
    }

    return status::StatusOK;
}

// Implementation of inbound_writer().write()
status::StatusCode SenderEndpoint::write(const packet::PacketPtr& packet) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(!packet);
    roc_panic_if(!parser_);

    state_tracker_.register_packet();
    inbound_queue_.push_back(*packet);

    return status::StatusOK;
}

} // namespace pipeline
} // namespace roc
