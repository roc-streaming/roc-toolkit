/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_fec/reader.h"
#include "roc_core/log.h"
#include "roc_core/macros.h"
#include "roc_core/panic.h"

namespace roc {
namespace fec {

namespace {

inline bool seqnum_lt(packet::seqnum_t a, packet::seqnum_t b) {
    return ROC_UNSIGNED_LT(packet::signed_seqnum_t, a, b);
}

inline bool seqnum_le(packet::seqnum_t a, packet::seqnum_t b) {
    return ROC_UNSIGNED_LE(packet::signed_seqnum_t, a, b);
}

inline packet::seqnum_t seqnum_sub(packet::seqnum_t a, packet::seqnum_t b) {
    return (packet::seqnum_t)ROC_UNSIGNED_SUB(packet::signed_seqnum_t, a, b);
}

} // namespace

Reader::Reader(const Config& config,
               IDecoder& decoder,
               packet::IReader& source_reader,
               packet::IReader& repair_reader,
               packet::IParser& parser,
               packet::PacketPool& packet_pool,
               core::IAllocator& allocator)
    : decoder_(decoder)
    , source_reader_(source_reader)
    , repair_reader_(repair_reader)
    , parser_(parser)
    , packet_pool_(packet_pool)
    , source_queue_(0)
    , repair_queue_(0)
    , source_block_(allocator, config.n_source_packets)
    , repair_block_(allocator, config.n_repair_packets)
    , is_alive_(true)
    , is_started_(false)
    , can_repair_(false)
    , next_packet_(0)
    , cur_block_sn_(0)
    , has_source_(false)
    , source_(0)
    , n_packets_(0) {
    source_block_.resize(source_block_.max_size());
    repair_block_.resize(repair_block_.max_size());
}

bool Reader::is_started() const {
    return is_started_;
}

bool Reader::is_alive() const {
    return is_alive_;
}

packet::PacketPtr Reader::read() {
    if (!is_alive_) {
        return NULL;
    }
    packet::PacketPtr pp = read_();
    if (pp) {
        n_packets_++;
    }
    // Check if is_alive_ have changed.
    return (is_alive_ ? pp : NULL);
}

packet::PacketPtr Reader::read_() {
    fetch_packets_();

    if (!is_started_) {
        packet::PacketPtr pp = source_queue_.head();
        if (pp) {
            if (!has_source_) {
                source_ = pp->rtp()->source;
                has_source_ = true;
            }
            cur_block_sn_ = pp->rtp()->seqnum;
            skip_repair_packets_();
        }

        if (!pp || !pp->rtp()->marker) {
            return source_queue_.read();
        }

        roc_log(LogInfo, "fec reader: got marker bit, start decoding:"
                         " n_packets_before=%u blk_sn=%lu",
                n_packets_, (unsigned long)cur_block_sn_);

        is_started_ = true;
    }

    return get_next_packet_();
}

packet::PacketPtr Reader::get_next_packet_() {
    update_packets_();

    packet::PacketPtr pp = source_block_[next_packet_];

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

void Reader::next_block_() {
    roc_log(LogTrace, "fec reader: next block: sn=%lu", (unsigned long)cur_block_sn_);

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

void Reader::try_repair_() {
    if (!can_repair_) {
        return;
    }

    for (size_t n = 0; n < source_block_.size(); n++) {
        if (!source_block_[n]) {
            continue;
        }
        decoder_.set(n, source_block_[n]->fec()->payload);
    }

    for (size_t n = 0; n < repair_block_.size(); n++) {
        if (!repair_block_[n]) {
            continue;
        }
        decoder_.set(source_block_.size() + n, repair_block_[n]->fec()->payload);
    }

    for (size_t n = 0; n < source_block_.size(); n++) {
        if (source_block_[n]) {
            continue;
        }

        core::Slice<uint8_t> buffer = decoder_.repair(n);
        if (!buffer) {
            continue;
        }

        packet::PacketPtr pp = new (packet_pool_) packet::Packet(packet_pool_);
        if (!pp) {
            roc_log(LogError, "fec reader: can't allocate packet");
            continue;
        }

        if (!parser_.parse(*pp, buffer)) {
            roc_log(LogDebug, "fec reader: can't parse repaired packet");
            continue;
        }

        pp->set_data(buffer);

        if (!check_packet_(pp, n)) {
            roc_log(LogDebug, "fec reader: dropping unexpected repaired packet");
            continue;
        }

        source_block_[n] = pp;
    }

    decoder_.reset();
    can_repair_ = false;
}

bool Reader::check_packet_(const packet::PacketPtr& pp, size_t pos) {
    roc_panic_if_not(has_source_);

    if (!pp->rtp()) {
        roc_log(LogTrace, "fec reader: repaired unexcpeted non-rtp packet");
        return false;
    }

    if (pp->rtp()->source != source_) {
        roc_log(LogTrace, "fec reader: repaired packet has bad source id, shutting down:"
                          " got=%lu expected=%lu",
                (unsigned long)pp->rtp()->source, (unsigned long)source_);
        return (is_alive_ = false);
    }

    if (pp->rtp()->seqnum != packet::seqnum_t(cur_block_sn_ + pos)) {
        roc_log(LogTrace,
                "fec reader: repaired packet has bad seqnum: got=%lu expected=%lu",
                (unsigned long)pp->rtp()->seqnum,
                (unsigned long)packet::seqnum_t(cur_block_sn_ + pos));
        return false;
    }

    return true;
}

void Reader::fetch_packets_() {
    while (source_queue_.size() <= source_block_.size() * 2) {
        if (packet::PacketPtr pp = source_reader_.read()) {
            if (!pp->rtp()) {
                roc_panic("fec reader: unexpected non-rtp source packet");
            }
            if (!pp->fec()) {
                roc_panic("fec reader: unexpected non-fec source packet");
            }
            source_queue_.write(pp);
        } else {
            break;
        }
    }

    while (repair_queue_.size() <= repair_block_.size() * 2) {
        if (packet::PacketPtr pp = repair_reader_.read()) {
            if (!pp->fec()) {
                roc_panic("fec reader: unexpected non-fec repair packet");
            }
            repair_queue_.write(pp);
        } else {
            break;
        }
    }
}

void Reader::update_packets_() {
    update_source_packets_();
    update_repair_packets_();
}

void Reader::update_source_packets_() {
    unsigned n_fetched = 0, n_added = 0, n_dropped = 0;

    for (;;) {
        packet::PacketPtr pp = source_queue_.head();
        if (!pp) {
            break;
        }

        const packet::RTP* rtp = pp->rtp();
        if (!rtp) {
            roc_panic("fec reader: unexpected non-rtp source packet");
        }

        if (!seqnum_lt(rtp->seqnum,
                       packet::seqnum_t(cur_block_sn_ + source_block_.size()))) {
            break;
        }

        source_queue_.read();
        n_fetched++;

        if (seqnum_lt(rtp->seqnum, cur_block_sn_)) {
            roc_log(LogDebug, "fec reader: dropping source packet from previous block:"
                              " blk_sn=%lu pkt_sn=%lu",
                    (unsigned long)cur_block_sn_, (unsigned long)rtp->seqnum);
            n_dropped++;
            continue;
        }

        const size_t p_num = seqnum_sub(rtp->seqnum, cur_block_sn_);

        if (!source_block_[p_num]) {
            can_repair_ = true;
            source_block_[p_num] = pp;
            n_added++;
        }
    }

    if (n_dropped != 0 || n_fetched != n_added) {
        roc_log(LogInfo, "fec reader: source queue: fetched=%u added=%u dropped=%u",
                n_fetched, n_added, n_dropped);
    }
}

void Reader::update_repair_packets_() {
    unsigned n_fetched = 0, n_added = 0, n_dropped = 0;

    for (;;) {
        packet::PacketPtr pp = repair_queue_.head();
        if (!pp) {
            break;
        }

        const packet::RTP* rtp = pp->rtp();
        if (!rtp) {
            roc_panic("fec reader: unexpected non-rtp repair packet");
        }

        const packet::FEC* fec = pp->fec();
        if (!fec) {
            roc_panic("fec reader: unexpected non-fec repair packet");
        }

        if (!seqnum_lt(fec->source_blknum,
                       packet::seqnum_t(cur_block_sn_ + source_block_.size()))) {
            break;
        }

        repair_queue_.read();
        n_fetched++;

        if (seqnum_lt(fec->source_blknum, cur_block_sn_)) {
            roc_log(LogDebug, "fec reader: dropping repair packet from previous block:"
                              " blk_sn=%lu pkt_data_blk=%lu",
                    (unsigned long)cur_block_sn_, (unsigned long)fec->source_blknum);
            n_dropped++;
            continue;
        }

        if (!seqnum_le(fec->repair_blknum, rtp->seqnum)) {
            roc_log(LogDebug, "fec reader: dropping invalid repair packet:"
                              " pkt_sn=%lu pkt_fec_blk=%lu",
                    (unsigned long)rtp->seqnum, (unsigned long)fec->repair_blknum);
            n_dropped++;
            continue;
        }

        const size_t p_num = seqnum_sub(rtp->seqnum, fec->repair_blknum);

        if (!repair_block_[p_num]) {
            can_repair_ = true;
            repair_block_[p_num] = pp;
            n_added++;
        }
    }

    if (n_dropped != 0 || n_fetched != n_added) {
        roc_log(LogInfo, "fec reader: repair queue: fetched=%u added=%u dropped=%u",
                n_fetched, n_added, n_dropped);
    }
}

void Reader::skip_repair_packets_() {
    unsigned n_skipped = 0;

    for (;;) {
        packet::PacketPtr pp = repair_queue_.head();
        if (!pp) {
            break;
        }

        const packet::FEC* fec = pp->fec();
        if (!fec) {
            roc_panic("fec reader: unexpected non-fec repair packet");
        }

        if (!seqnum_lt(fec->source_blknum, cur_block_sn_)) {
            break;
        }

        roc_log(LogDebug, "fec reader: dropping repair packet, decoding not started:"
                          " min_sn=%lu pkt_data_blk=%lu",
                (unsigned long)cur_block_sn_, (unsigned long)fec->source_blknum);

        repair_queue_.read();
        n_skipped++;
    }

    if (n_skipped != 0) {
        roc_log(LogInfo, "fec reader: repair queue: skipped=%u", n_skipped);
    }
}

} // namespace fec
} // namespace roc
