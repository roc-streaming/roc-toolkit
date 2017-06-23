/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/random.h"

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
    , cur_block_source_sn_(0)
    , cur_block_repair_sn_((packet::seqnum_t)core::random(packet::seqnum_t(-1)))
    , cur_source_packet_n_(0) {
}

void Encoder::write(const packet::IPacketPtr& pp) {
    roc_panic_if_not(pp);

    if (!pp->rtp()) {
        roc_panic("fec encoder: unexpected packet w/o RTP header");
    }

    if (first_packet_) {
        first_packet_ = false;
        do {
            source_ = (packet::source_t)core::random(packet::source_t(-1));
        } while (source_ == pp->rtp()->source());
    }

    if (cur_source_packet_n_ == 0) {
        cur_block_source_sn_ = pp->rtp()->seqnum();
        pp->rtp()->set_marker(true);
    }

    packet_output_.write(pp);

    block_encoder_.write(cur_source_packet_n_, pp->raw_data());

    if (++cur_source_packet_n_ >= block_encoder_.n_source_packets()) {
        // Encode repair packet of this block.
        block_encoder_.commit();

        // Send repair packets.
        for (packet::seqnum_t i = 0; i < block_encoder_.n_repair_packets(); ++i) {
            packet::IPacketPtr rp = make_repair_packet_(
                block_encoder_.read(i), cur_block_source_sn_, cur_block_repair_sn_,
                cur_block_repair_sn_ + i, i == 0);

            if (rp) {
                packet_output_.write(rp);
            } else {
                roc_log(LogDebug, "fec encoder: can't create fec packet");
            }
        }

        cur_block_repair_sn_ += block_encoder_.n_repair_packets();
        cur_source_packet_n_ = 0;

        block_encoder_.reset();
    }
}

packet::IPacketPtr Encoder::make_repair_packet_(const core::IByteBufferConstSlice& buff,
                                                const packet::seqnum_t blk_source_sn,
                                                const packet::seqnum_t blk_repair_sn,
                                                const packet::seqnum_t sn,
                                                const bool marker_bit) {
    if (!buff) {
        return NULL;
    }

    packet::IPacketPtr pp = packet_composer_.compose(packet::IPacket::HasFEC);

    roc_panic_if_not(pp->rtp());
    roc_panic_if_not(pp->fec());

    pp->rtp()->set_source(source_);
    pp->rtp()->set_seqnum(sn);
    pp->rtp()->set_marker(marker_bit);

    pp->fec()->set_source_blknum(blk_source_sn);
    pp->fec()->set_repair_blknum(blk_repair_sn);

    pp->set_payload(buff.data(), buff.size());

    return pp;
}

} // namespace fec
} // namespace roc
