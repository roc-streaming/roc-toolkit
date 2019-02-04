/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_fec/reader.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace fec {

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
    , source_block_(allocator)
    , repair_block_(allocator)
    , valid_(false)
    , alive_(true)
    , started_(false)
    , can_repair_(false)
    , next_packet_(0)
    , cur_sbn_(0)
    , has_source_(false)
    , source_(0)
    , n_packets_(0) {
    if (!source_block_.resize(config.n_source_packets)) {
        return;
    }
    if (!repair_block_.resize(config.n_repair_packets)) {
        return;
    }
    valid_ = true;
}

bool Reader::valid() const {
    return valid_;
}

bool Reader::started() const {
    return started_;
}

bool Reader::alive() const {
    return alive_;
}

packet::PacketPtr Reader::read() {
    roc_panic_if_not(valid());
    if (!alive_) {
        return NULL;
    }
    packet::PacketPtr pp = read_();
    if (pp) {
        n_packets_++;
    }
    // Check if alive_ have changed.
    return (alive_ ? pp : NULL);
}

packet::PacketPtr Reader::read_() {
    fetch_packets_();

    if (!started_) {
        packet::PacketPtr pp = source_queue_.head();
        if (pp) {
            if (!has_source_) {
                source_ = pp->rtp()->source;
                has_source_ = true;
            }
            cur_sbn_ = pp->fec()->source_block_number;
            drop_repair_packets_from_prev_blocks_();
        }

        if (!pp || pp->fec()->encoding_symbol_id > 0) {
            return source_queue_.read();
        }

        roc_log(LogDebug, "fec reader: got first packet in a block, start decoding:"
                          " n_packets_before=%u sn=%lu sbn=%lu",
                          n_packets_,
                          (unsigned long)pp->rtp()->seqnum,
                          (unsigned long)cur_sbn_);

        started_ = true;
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
    roc_log(LogTrace, "fec reader: next block: sbn=%lu", (unsigned long)cur_sbn_);

    for (size_t n = 0; n < source_block_.size(); n++) {
        source_block_[n] = NULL;
    }

    for (size_t n = 0; n < repair_block_.size(); n++) {
        repair_block_[n] = NULL;
    }

    cur_sbn_++;
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

        if (!check_packet_(pp)) {
            roc_log(LogDebug, "fec reader: dropping unexpected repaired packet");
            continue;
        }

        source_block_[n] = pp;
    }

    decoder_.reset();
    can_repair_ = false;
}

bool Reader::check_packet_(const packet::PacketPtr& pp) {
    roc_panic_if_not(has_source_);

    if (!pp->rtp()) {
        roc_log(LogDebug, "fec reader: repaired unexpected non-rtp packet");
        return false;
    }

    if (pp->rtp()->source != source_) {
        roc_log(LogDebug,
                "fec reader: repaired packet has bad source id, shutting down:"
                " got=%lu expected=%lu",
                (unsigned long)pp->rtp()->source, (unsigned long)source_);
        return (alive_ = false);
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
        const packet::FEC* fec = pp->fec();
        if (!rtp) {
            roc_panic("fec reader: unexpected non-rtp source packet");
        }

        if (!packet::blknum_le(fec->source_block_number, cur_sbn_)) {
            break;
        }

        source_queue_.read();
        n_fetched++;

        if (packet::blknum_lt(fec->source_block_number, cur_sbn_)) {
            roc_log(LogTrace, "fec reader: dropping source packet from previous block:"
                              " cur_sbn=%lu pkt_sbn=%lu pkt_sn=%lu",
                              (unsigned long)cur_sbn_,
                              (unsigned long)fec->source_block_number,
                              (unsigned long)rtp->seqnum);
            n_dropped++;
            continue;
        }

        roc_panic_if(fec->source_block_number != cur_sbn_);
        const size_t p_num = fec->encoding_symbol_id;

        if (!source_block_[p_num]) {
            can_repair_ = true;
            source_block_[p_num] = pp;
            n_added++;
        }
    }

    if (n_dropped != 0 || n_fetched != n_added) {
        roc_log(LogDebug, "fec reader: source queue: fetched=%u added=%u dropped=%u",
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

        const packet::FEC* fec = pp->fec();
        if (!fec) {
            roc_panic("fec reader: unexpected non-fec repair packet");
        }

        if (!packet::blknum_le(fec->source_block_number, cur_sbn_)) {
            break;
        }

        repair_queue_.read();
        n_fetched++;

        if (packet::blknum_lt(fec->source_block_number, cur_sbn_)) {
            roc_log(LogTrace, "fec reader: dropping repair packet from previous block:"
                              " cur_sbn=%lu pkt_sbn=%lu",
                              (unsigned long)cur_sbn_,
                              (unsigned long)fec->source_block_number);
            n_dropped++;
            continue;
        }

        roc_panic_if(fec->encoding_symbol_id < fec->source_block_length);
        const size_t p_num = fec->encoding_symbol_id - fec->source_block_length;
        roc_panic_if(p_num >= repair_block_.size());

        if (!repair_block_[p_num]) {
            can_repair_ = true;
            repair_block_[p_num] = pp;
            n_added++;
        }
    }

    if (n_dropped != 0 || n_fetched != n_added) {
        roc_log(LogDebug, "fec reader: repair queue: fetched=%u added=%u dropped=%u",
                n_fetched, n_added, n_dropped);
    }
}

void Reader::drop_repair_packets_from_prev_blocks_() {
    unsigned n_dropped = 0;

    for (;;) {
        packet::PacketPtr pp = repair_queue_.head();
        if (!pp) {
            break;
        }

        const packet::FEC* fec = pp->fec();
        if (!fec) {
            roc_panic("fec reader: unexpected non-fec repair packet");
        }

        if (!packet::blknum_lt(fec->source_block_number, cur_sbn_)) {
            break;
        }

        roc_log(LogTrace,
                "fec reader: dropping repair packet from previous blocks,"
                          " decoding not started: cur_sbn=%lu pkt_sbn=%lu",
                          (unsigned long)cur_sbn_,
                          (unsigned long)fec->source_block_number);

        repair_queue_.read();
        n_dropped++;
    }

    if (n_dropped != 0) {
        roc_log(LogDebug, "fec reader: repair queue: dropped=%u", n_dropped);
    }
}

} // namespace fec
} // namespace roc
