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

void Encoder::write(const packet::IPacketPtr& pp) {
    roc_panic_if_not(pp);

    if (!pp->rtp()) {
        roc_panic("encoder: unexpected packet w/o RTP header");
    }

    if (first_packet_) {
        first_packet_ = false;
        do {
            source_ = (packet::source_t)core::random(packet::source_t(-1));
        } while (source_ == pp->rtp()->source());
    }

    if (cur_data_pack_i_ == 0) {
        cur_block_seqnum_ = pp->rtp()->seqnum();
        pp->rtp()->set_marker(true);
    }

    packet_output_.write(pp);

    block_encoder_.write(cur_data_pack_i_, pp->raw_data());

    if (++cur_data_pack_i_ >= block_encoder_.n_data_packets()) {
        // Calculate redundant packet of this block.
        block_encoder_.commit();

        // Send redundant packets.
        for (packet::seqnum_t i = 0; i < block_encoder_.n_fec_packets(); ++i) {
            packet::IPacketPtr fp = make_fec_packet_(
                block_encoder_.read(i), cur_block_seqnum_, cur_session_fec_seqnum_,
                cur_session_fec_seqnum_ + i, i == 0);

            if (fp) {
                packet_output_.write(fp);
            } else {
                roc_log(LogDebug, "fec encoder: can't create fec packet");
            }
        }
        cur_session_fec_seqnum_ += block_encoder_.n_fec_packets();
        cur_data_pack_i_ = 0;

        block_encoder_.reset();
    }
}

packet::IPacketPtr Encoder::make_fec_packet_(const core::IByteBufferConstSlice& buff,
                                             const packet::seqnum_t block_data_seqnum,
                                             const packet::seqnum_t block_fec_seqnum,
                                             const packet::seqnum_t seqnum,
                                             const bool marker_bit) {
    if (!buff) {
        return NULL;
    }

    packet::IPacketPtr pp = packet_composer_.compose(packet::IPacket::HasFEC);

    roc_panic_if_not(pp->rtp());
    roc_panic_if_not(pp->fec());

    pp->rtp()->set_source(source_);
    pp->rtp()->set_seqnum(seqnum);
    pp->rtp()->set_marker(marker_bit);

    pp->fec()->set_data_blknum(block_data_seqnum);
    pp->fec()->set_fec_blknum(block_fec_seqnum);

    pp->set_payload(buff.data(), buff.size());

    return pp;
}

} // namespace fec
} // namespace roc
