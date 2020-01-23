/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_endpoint.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_fec/composer.h"
#include "roc_fec/headers.h"

namespace roc {
namespace pipeline {

SenderEndpoint::SenderEndpoint(address::EndpointProtocol proto,
                               core::IAllocator& allocator)
    : proto_(proto)
    , writer_(NULL)
    , composer_(NULL) {
    packet::IComposer* composer = NULL;

    switch ((int)proto) {
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

    switch ((int)proto) {
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

bool SenderEndpoint::valid() const {
    return composer_;
}

address::EndpointProtocol SenderEndpoint::proto() const {
    roc_panic_if(!valid());

    return proto_;
}

packet::IComposer& SenderEndpoint::composer() {
    roc_panic_if(!valid());

    return *composer_;
}

bool SenderEndpoint::has_writer() const {
    core::Mutex::Lock lock(mutex_);

    return writer_;
}

void SenderEndpoint::set_output_writer(packet::IWriter& writer) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());
    roc_panic_if(writer_);

    writer_ = &writer;
}

void SenderEndpoint::set_destination_udp_address(const address::SocketAddr& addr) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());
    roc_panic_if(udp_address_.has_host_port());

    udp_address_ = addr;
}

void SenderEndpoint::write(const packet::PacketPtr& packet) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    if (!writer_) {
        return;
    }

    if (udp_address_.has_host_port()) {
        packet->add_flags(packet::Packet::FlagUDP);
        packet->udp()->dst_addr = udp_address_;
    }

    if ((packet->flags() & packet::Packet::FlagComposed) == 0) {
        if (!composer_->compose(*packet)) {
            roc_panic("sender endpoint: can't compose packet");
        }
        packet->add_flags(packet::Packet::FlagComposed);
    }

    writer_->write(packet);
}

} // namespace pipeline
} // namespace roc
