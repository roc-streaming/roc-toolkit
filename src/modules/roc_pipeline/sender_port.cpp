/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_port.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_fec/composer.h"
#include "roc_fec/headers.h"

namespace roc {
namespace pipeline {

SenderPort::SenderPort(const PortConfig& config,
                       packet::IWriter& writer,
                       core::IAllocator& allocator)
    : dst_address_(config.address)
    , writer_(writer)
    , composer_(NULL) {
    packet::IComposer* composer = NULL;

    switch ((unsigned)config.protocol) {
    case address::EndProto_RTP:
    case address::EndProto_RTP_LDPC_Source:
    case address::EndProto_RTP_RS8M_Source:
        rtp_composer_.reset(new (allocator) rtp::Composer(NULL), allocator);
        if (!rtp_composer_) {
            return;
        }
        composer = rtp_composer_.get();
        break;
    }

    switch ((unsigned)config.protocol) {
    case address::EndProto_RTP_LDPC_Source:
        fec_composer_.reset(
            new (allocator)
                fec::Composer<fec::LDPC_Source_PayloadID, fec::Source, fec::Footer>(
                    composer),
            allocator);
        if (!fec_composer_) {
            return;
        }
        composer = fec_composer_.get();
        break;
    case address::EndProto_LDPC_Repair:
        fec_composer_.reset(
            new (allocator)
                fec::Composer<fec::LDPC_Repair_PayloadID, fec::Repair, fec::Header>(
                    composer),
            allocator);
        if (!fec_composer_) {
            return;
        }
        composer = fec_composer_.get();
        break;
    case address::EndProto_RTP_RS8M_Source:
        fec_composer_.reset(
            new (allocator)
                fec::Composer<fec::RS8M_PayloadID, fec::Source, fec::Footer>(composer),
            allocator);
        if (!fec_composer_) {
            return;
        }
        composer = fec_composer_.get();
        break;
    case address::EndProto_RS8M_Repair:
        fec_composer_.reset(
            new (allocator)
                fec::Composer<fec::RS8M_PayloadID, fec::Repair, fec::Header>(composer),
            allocator);
        if (!fec_composer_) {
            return;
        }
        composer = fec_composer_.get();
        break;
    }

    composer_ = composer;
}

bool SenderPort::valid() const {
    return composer_;
}

packet::IComposer& SenderPort::composer() {
    roc_panic_if(!valid());

    return *composer_;
}

void SenderPort::write(const packet::PacketPtr& packet) {
    roc_panic_if(!valid());

    packet->add_flags(packet::Packet::FlagUDP);

    packet::UDP& udp = *packet->udp();

    udp.dst_addr = dst_address_;

    if ((packet->flags() & packet::Packet::FlagComposed) == 0) {
        if (!composer_->compose(*packet)) {
            roc_panic("sender port: can't compose packet");
        }
        packet->add_flags(packet::Packet::FlagComposed);
    }

    writer_.write(packet);
}

} // namespace pipeline
} // namespace roc
