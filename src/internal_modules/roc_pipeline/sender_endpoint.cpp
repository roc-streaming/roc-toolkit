/*
 * Copyright (c) 2017 Roc Streaming authors
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

SenderEndpoint::SenderEndpoint(address::Protocol proto,
                               const address::SocketAddr& dest_address,
                               packet::IWriter& dest_writer,
                               core::IArena& arena)
    : proto_(proto)
    , composer_(NULL) {
    packet::IComposer* composer = NULL;

    switch (proto) {
    case address::Proto_RTP:
    case address::Proto_RTP_LDPC_Source:
    case address::Proto_RTP_RS8M_Source:
        rtp_composer_.reset(new (rtp_composer_) rtp::Composer(NULL));
        if (!rtp_composer_) {
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
                    composer),
            arena);
        if (!fec_composer_) {
            return;
        }
        composer = fec_composer_.get();
        break;
    case address::Proto_LDPC_Repair:
        fec_composer_.reset(
            new (arena)
                fec::Composer<fec::LDPC_Repair_PayloadID, fec::Repair, fec::Header>(
                    composer),
            arena);
        if (!fec_composer_) {
            return;
        }
        composer = fec_composer_.get();
        break;
    case address::Proto_RTP_RS8M_Source:
        fec_composer_.reset(
            new (arena)
                fec::Composer<fec::RS8M_PayloadID, fec::Source, fec::Footer>(composer),
            arena);
        if (!fec_composer_) {
            return;
        }
        composer = fec_composer_.get();
        break;
    case address::Proto_RS8M_Repair:
        fec_composer_.reset(
            new (arena)
                fec::Composer<fec::RS8M_PayloadID, fec::Repair, fec::Header>(composer),
            arena);
        if (!fec_composer_) {
            return;
        }
        composer = fec_composer_.get();
        break;
    default:
        break;
    }

    switch (proto) {
    case address::Proto_RTCP:
        rtcp_composer_.reset(new (rtcp_composer_) rtcp::Composer());
        if (!rtcp_composer_) {
            return;
        }
        composer = rtcp_composer_.get();
        break;
    default:
        break;
    }

    composer_ = composer;
    if (!composer_) {
        return;
    }

    packet_shipper_.reset(new (packet_shipper_)
                              packet::Shipper(dest_address, *composer_, dest_writer));
}

bool SenderEndpoint::is_valid() const {
    return composer_ && packet_shipper_;
}

address::Protocol SenderEndpoint::proto() const {
    roc_panic_if(!is_valid());

    return proto_;
}

packet::IComposer& SenderEndpoint::composer() {
    roc_panic_if(!is_valid());

    return *composer_;
}

packet::IWriter& SenderEndpoint::writer() {
    roc_panic_if(!is_valid());

    return *packet_shipper_;
}

} // namespace pipeline
} // namespace roc
