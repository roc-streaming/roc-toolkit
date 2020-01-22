/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_port.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_fec/composer.h"
#include "roc_fec/headers.h"
#include "roc_fec/parser.h"

namespace roc {
namespace pipeline {

ReceiverPort::ReceiverPort(const PortConfig& config,
                           const rtp::FormatMap& format_map,
                           core::IAllocator& allocator)
    : allocator_(allocator)
    , config_(config)
    , parser_(NULL) {
    packet::IParser* parser = NULL;

    switch ((unsigned)config.protocol) {
    case address::EndProto_RTP:
    case address::EndProto_RTP_LDPC_Source:
    case address::EndProto_RTP_RS8M_Source:
        rtp_parser_.reset(new (allocator) rtp::Parser(format_map, NULL), allocator);
        if (!rtp_parser_) {
            return;
        }
        parser = rtp_parser_.get();
        break;
    }

    switch ((unsigned)config.protocol) {
    case address::EndProto_RTP_LDPC_Source:
        fec_parser_.reset(
            new (allocator)
                fec::Parser<fec::LDPC_Source_PayloadID, fec::Source, fec::Footer>(parser),
            allocator);
        if (!fec_parser_) {
            return;
        }
        parser = fec_parser_.get();
        break;
    case address::EndProto_LDPC_Repair:
        fec_parser_.reset(
            new (allocator)
                fec::Parser<fec::LDPC_Repair_PayloadID, fec::Repair, fec::Header>(parser),
            allocator);
        if (!fec_parser_) {
            return;
        }
        parser = fec_parser_.get();
        break;
    case address::EndProto_RTP_RS8M_Source:
        fec_parser_.reset(
            new (allocator)
                fec::Parser<fec::RS8M_PayloadID, fec::Source, fec::Footer>(parser),
            allocator);
        if (!fec_parser_) {
            return;
        }
        parser = fec_parser_.get();
        break;
    case address::EndProto_RS8M_Repair:
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

void ReceiverPort::destroy() {
    allocator_.destroy(*this);
}

bool ReceiverPort::valid() const {
    return parser_;
}

const PortConfig& ReceiverPort::config() const {
    return config_;
}

bool ReceiverPort::handle(packet::Packet& packet) {
    roc_panic_if(!valid());

    packet::UDP* udp = packet.udp();
    if (!udp) {
        return false;
    }

    if (udp->dst_addr != config_.address) {
        return false;
    }

    if (!parser_->parse(packet, packet.data())) {
        roc_log(LogDebug, "receiver port: failed to parse packet");
        return false;
    }

    return true;
}

} // namespace pipeline
} // namespace roc
