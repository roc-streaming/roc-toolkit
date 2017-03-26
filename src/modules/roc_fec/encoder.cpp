/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/random.h"
#include "roc_core/panic.h"
#include "roc_core/log.h"

#include "roc_fec/encoder.h"

namespace roc {
namespace fec {

Encoder::Encoder(IBlockEncoder& block_encoder,
                 packet::IPacketWriter& output,
                 packet::IPacketComposer& composer)
    : block_encoder_(block_encoder)
    , packet_output_(output)
    , packet_composer_(composer)
    , source_(0)
    , first_packet_(true)
    , cur_block_seqnum_(0)
    , cur_session_fec_seqnum_((packet::seqnum_t)core::random(packet::seqnum_t(-1)))
    , cur_data_pack_i_(0) {
}

void Encoder::write(const packet::IPacketPtr& p) {
    roc_panic_if_not(p);

    if (first_packet_) {
        first_packet_ = false;
        do {
            source_ = (packet::source_t)core::random(packet::source_t(-1));
        } while (source_ == p->source());
    }

    if (cur_data_pack_i_ == 0) {
        cur_block_seqnum_ = p->seqnum();
        p->set_marker(true);
    }

    packet_output_.write(p);

    block_encoder_.write(cur_data_pack_i_, p->raw_data());

    if (++cur_data_pack_i_ >= N_DATA_PACKETS) {
        // Calculate redundant packet of this block.
        block_encoder_.commit();

        // Send redundant packets.
        for (packet::seqnum_t i = 0; i < N_FEC_PACKETS; ++i) {
            packet::IFECPacketPtr fec_p = make_fec_packet_(
                block_encoder_.read(i), cur_block_seqnum_, cur_session_fec_seqnum_,
                cur_session_fec_seqnum_ + i, i == 0);

            if (fec_p) {
                packet_output_.write(fec_p);
            } else {
                roc_log(LogDebug, "fec encoder: can't create fec packet");
            }
        }
        cur_session_fec_seqnum_ += N_FEC_PACKETS;
        cur_data_pack_i_ = 0;

        block_encoder_.reset();
    }
}

packet::IFECPacketPtr Encoder::make_fec_packet_(const core::IByteBufferConstSlice& buff,
                                                const packet::seqnum_t block_data_seqnum,
                                                const packet::seqnum_t block_fec_seqnum,
                                                const packet::seqnum_t seqnum,
                                                const bool marker_bit) {
    if (!buff) {
        return NULL;
    }

    packet::IPacketPtr p = packet_composer_.compose(packet::IFECPacket::Type);

    roc_panic_if(p->type() != packet::IFECPacket::Type);

    if (packet::IFECPacketPtr fec_p = static_cast<packet::IFECPacket*>(p.get())) {
        fec_p->set_source(source_);
        fec_p->set_seqnum(seqnum);
        fec_p->set_marker(marker_bit);
        fec_p->set_data_blknum(block_data_seqnum);
        fec_p->set_fec_blknum(block_fec_seqnum);
        fec_p->set_payload(buff.data(), buff.size());
        return fec_p;
    } else {
        return NULL;
    }
}

} // namespace fec
} // namespace roc
