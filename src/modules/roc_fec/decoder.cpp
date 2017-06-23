/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/helpers.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

#include "roc_fec/decoder.h"

#define SEQ_IS_BEFORE(a, b) ROC_IS_BEFORE(packet::signed_seqnum_t, a, b)
#define SEQ_IS_BEFORE_EQ(a, b) ROC_IS_BEFORE_EQ(packet::signed_seqnum_t, a, b)
#define SEQ_SUBTRACT(a, b) (packet::seqnum_t) ROC_SUBTRACT(packet::signed_seqnum_t, a, b)

namespace roc {
namespace fec {

Decoder::Decoder(IBlockDecoder& block_decoder,
                 packet::IPacketReader& source_reader,
                 packet::IPacketReader& repair_reader,
                 packet::IPacketParser& parser)
    : block_decoder_(block_decoder)
    , source_reader_(source_reader)
    , repair_reader_(repair_reader)
    , parser_(parser)
    , source_queue_(0)
    , repair_queue_(0)
    , source_block_(block_decoder_.n_source_packets())
    , repair_block_(block_decoder_.n_repair_packets())
    , is_alive_(true)
    , is_started_(false)
    , can_repair_(false)
    , next_packet_(0)
    , cur_block_sn_(0)
    , has_source_(false)
    , source_(0)
    , n_packets_(0) {
}

bool Decoder::is_started() const {
    return is_started_;
}

bool Decoder::is_alive() const {
    return is_alive_;
}

packet::IPacketConstPtr Decoder::read() {
    if (!is_alive_) {
        return NULL;
    }
    packet::IPacketConstPtr pp = read_();
    if (pp) {
        n_packets_++;
    }
    // Check if is_alive_ changed.
    return (is_alive_ ? pp : NULL);
}

packet::IPacketConstPtr Decoder::read_() {
    fetch_packets_();

    if (!is_started_) {
        packet::IPacketConstPtr pp = source_queue_.head();
        if (pp) {
            if (!has_source_) {
                source_ = pp->rtp()->source();
                has_source_ = true;
            }
            cur_block_sn_ = pp->rtp()->seqnum();
            skip_repair_packets_();
        }

        if (!pp || !pp->rtp()->marker()) {
            return source_queue_.read();
        }

        roc_log(LogInfo, "fec decoder: got marker bit, start decoding:"
                         " n_packets_before=%u blk_sn=%lu",
                n_packets_, (unsigned long)cur_block_sn_);

        is_started_ = true;
    }

    return get_next_packet_();
}

packet::IPacketConstPtr Decoder::get_next_packet_() {
    update_packets_();

    packet::IPacketConstPtr pp = source_block_[next_packet_];

    do {
        if (!pp) {
            try_repair_();

            size_t pos;
            for (pos = next_packet_; pos < source_block_.size(); pos++) {
                if (source_block_[pos]) {
                    break;
                }
            }

            if (pos == source_block_.size()) {
                if (source_queue_.size() == 0) {
                    return NULL;
                }
            } else {
                pp = source_block_[pos++];
            }

            next_packet_ = pos;
        } else {
            next_packet_++;
        }

        if (next_packet_ == source_block_.size()) {
            next_block_();
        }
    } while (!pp);

    return pp;
}

void Decoder::next_block_() {
    roc_log(LogTrace, "fec decoder: next block: sn=%lu", (unsigned long)cur_block_sn_);

    for (size_t n = 0; n < source_block_.size(); n++) {
        source_block_[n] = NULL;
    }

    for (size_t n = 0; n < repair_block_.size(); n++) {
        repair_block_[n] = NULL;
    }

    cur_block_sn_ += source_block_.size();
    next_packet_ = 0;

    can_repair_ = false;
    update_packets_();
}

void Decoder::try_repair_() {
    if (!can_repair_) {
        return;
    }

    for (size_t n = 0; n < source_block_.size(); n++) {
        if (!source_block_[n]) {
            continue;
        }
        block_decoder_.write(n, source_block_[n]->raw_data());
    }

    for (size_t n = 0; n < repair_block_.size(); n++) {
        if (!repair_block_[n]) {
            continue;
        }
        block_decoder_.write(source_block_.size() + n, repair_block_[n]->payload());
    }

    for (size_t n = 0; n < source_block_.size(); n++) {
        if (source_block_[n]) {
            continue;
        }

        core::IByteBufferConstSlice buffer = block_decoder_.repair(n);
        if (!buffer) {
            continue;
        }

        packet::IPacketConstPtr pp = parser_.parse(buffer);
        if (!pp) {
            roc_log(LogDebug, "fec decoder: dropping unparsable repaired packet");
            continue;
        }

        if (!check_packet_(pp, n)) {
            roc_log(LogDebug, "fec decoder: dropping unexpected repaired packet");
            continue;
        }

        source_block_[n] = pp;
    }

    block_decoder_.reset();
    can_repair_ = false;
}

bool Decoder::check_packet_(const packet::IPacketConstPtr& pp, size_t pos) {
    roc_panic_if_not(has_source_);

    if (!pp->rtp()) {
        roc_log(LogTrace, "fec decoder: repaired unexcpeted non-RTP packet");
        return false;
    }

    if (pp->rtp()->source() != source_) {
        roc_log(LogTrace, "fec decoder: repaired packet has bad source id, shutting down:"
                          " got=%lu expected=%lu",
                (unsigned long)pp->rtp()->source(), (unsigned long)source_);
        // We've repaired packet from someone else's session; shutdown decoder now.
        // This will force Watchdog to shutdown entire session after timeout.
        return (is_alive_ = false);
    }

    if (pp->rtp()->seqnum() != packet::seqnum_t(cur_block_sn_ + pos)) {
        roc_log(LogTrace,
                "fec decoder: repaired packet has bad seqnum: got=%lu expected=%lu",
                (unsigned long)pp->rtp()->seqnum(),
                (unsigned long)packet::seqnum_t(cur_block_sn_ + pos));
        return false;
    }

    return true;
}

void Decoder::fetch_packets_() {
    while (source_queue_.size() <= source_block_.size() * 2) {
        if (packet::IPacketConstPtr pp = source_reader_.read()) {
            source_queue_.write(pp);
        } else {
            break;
        }
    }

    while (repair_queue_.size() <= repair_block_.size() * 2) {
        if (packet::IPacketConstPtr pp = repair_reader_.read()) {
            repair_queue_.write(pp);
        } else {
            break;
        }
    }
}

void Decoder::update_packets_() {
    update_source_packets_();
    update_repair_packets_();
}

void Decoder::update_source_packets_() {
    unsigned n_fetched = 0, n_added = 0, n_dropped = 0;

    for (;;) {
        packet::IPacketConstPtr pp = source_queue_.head();
        if (!pp) {
            break;
        }

        const packet::IHeaderRTP* rtp = pp->rtp();
        if (!rtp) {
            roc_panic("fec decoder: unexpected data packet w/o RTP header");
        }

        if (!SEQ_IS_BEFORE(rtp->seqnum(), cur_block_sn_ + source_block_.size())) {
            break;
        }

        source_queue_.read();
        n_fetched++;

        if (SEQ_IS_BEFORE(rtp->seqnum(), cur_block_sn_)) {
            roc_log(LogDebug, "fec decoder: dropping data packet from previous block:"
                              " blk_sn=%lu pkt_sn=%lu",
                    (unsigned long)cur_block_sn_, (unsigned long)rtp->seqnum());
            n_dropped++;
            continue;
        }

        const size_t p_num = SEQ_SUBTRACT(rtp->seqnum(), cur_block_sn_);

        if (!source_block_[p_num]) {
            can_repair_ = true;
            source_block_[p_num] = pp;
            n_added++;
        }
    }

    if (n_dropped != 0 || n_fetched != n_added) {
        roc_log(LogInfo, "fec decoder: data queue: fetched=%u added=%u dropped=%u",
                n_fetched, n_added, n_dropped);
    }
}

void Decoder::update_repair_packets_() {
    unsigned n_fetched = 0, n_added = 0, n_dropped = 0;

    for (;;) {
        packet::IPacketConstPtr pp = repair_queue_.head();
        if (!pp) {
            break;
        }

        const packet::IHeaderRTP* rtp = pp->rtp();
        if (!rtp) {
            roc_panic("fec decoder: unexpected fec packet w/o RTP header");
        }

        const packet::IHeaderFECFrame* fec = pp->fec();
        if (!fec) {
            roc_panic("fec decoder: unexpected fec packet w/o FECFRAME header");
        }

        if (!SEQ_IS_BEFORE(fec->source_blknum(), cur_block_sn_ + source_block_.size())) {
            break;
        }

        repair_queue_.read();
        n_fetched++;

        if (SEQ_IS_BEFORE(fec->source_blknum(), cur_block_sn_)) {
            roc_log(LogDebug, "fec decoder: dropping fec packet from previous block:"
                              " blk_sn=%lu pkt_data_blk=%lu",
                    (unsigned long)cur_block_sn_, (unsigned long)fec->source_blknum());
            n_dropped++;
            continue;
        }

        if (!SEQ_IS_BEFORE_EQ(fec->repair_blknum(), rtp->seqnum())) {
            roc_log(LogDebug, "fec decoder: dropping invalid fec packet:"
                              " pkt_sn=%lu pkt_fec_blk=%lu",
                    (unsigned long)rtp->seqnum(), (unsigned long)fec->repair_blknum());
            n_dropped++;
            continue;
        }

        const size_t p_num = SEQ_SUBTRACT(rtp->seqnum(), fec->repair_blknum());

        if (!repair_block_[p_num]) {
            can_repair_ = true;
            repair_block_[p_num] = pp;
            n_added++;
        }
    }

    if (n_dropped != 0 || n_fetched != n_added) {
        roc_log(LogInfo, "fec decoder: fec queue: fetched=%u added=%u dropped=%u",
                n_fetched, n_added, n_dropped);
    }
}

void Decoder::skip_repair_packets_() {
    unsigned n_skipped = 0;

    for (;;) {
        packet::IPacketConstPtr pp = repair_queue_.head();
        if (!pp) {
            break;
        }

        const packet::IHeaderFECFrame* fec = pp->fec();
        if (!fec) {
            roc_panic("fec decoder: unexpected fec packet w/o FECFRAME header");
        }

        if (!SEQ_IS_BEFORE(fec->source_blknum(), cur_block_sn_)) {
            break;
        }

        roc_log(LogDebug, "fec decoder: dropping fec packet, decoding not started:"
                          " min_sn=%lu pkt_data_blk=%lu",
                (unsigned long)cur_block_sn_, (unsigned long)fec->source_blknum());

        repair_queue_.read();
        n_skipped++;
    }

    if (n_skipped != 0) {
        roc_log(LogInfo, "fec decoder: fec queue: skipped=%u", n_skipped);
    }
}

} // namespace fec
} // namespace roc
