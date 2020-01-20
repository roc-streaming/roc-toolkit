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
#include "roc_packet/fec_scheme_to_str.h"

namespace roc {
namespace fec {

Reader::Reader(const ReaderConfig& config,
               packet::FecScheme fec_scheme,
               IBlockDecoder& decoder,
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
    , payload_size_(0)
    , source_block_resized_(false)
    , repair_block_resized_(false)
    , payload_resized_(false)
    , n_packets_(0)
    , max_sbn_jump_(config.max_sbn_jump)
    , fec_scheme_(fec_scheme) {
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
    // check if alive_ has changed
    return (alive_ ? pp : NULL);
}

packet::PacketPtr Reader::read_() {
    fetch_packets_();

    if (!started_) {
        packet::PacketPtr pp = get_first_packet_();
        if (!pp || pp->fec()->encoding_symbol_id > 0) {
            return source_queue_.read();
        }

        roc_log(LogDebug,
                "fec reader: got first packet in a block, start decoding:"
                " n_packets_before=%u sbn=%lu",
                n_packets_, (unsigned long)cur_sbn_);

        started_ = true;
    }

    return get_next_packet_();
}

packet::PacketPtr Reader::get_first_packet_() {
    packet::PacketPtr pp = source_queue_.head();
    if (!pp) {
        return NULL;
    }

    const packet::FEC& fec = *pp->fec();

    if (!process_source_packet_(pp)) {
        roc_log(LogTrace,
                "fec reader: dropping leading source packet:"
                " esi=%lu sblen=%lu blen=%lu payload_size=%lu",
                (unsigned long)fec.encoding_symbol_id,
                (unsigned long)fec.source_block_length, (unsigned long)fec.block_length,
                (unsigned long)fec.payload.size());
        return NULL;
    }

    cur_sbn_ = fec.source_block_number;
    drop_repair_packets_from_prev_blocks_();

    return pp;
}

packet::PacketPtr Reader::get_next_packet_() {
    fill_block_();

    packet::PacketPtr pp = source_block_[next_packet_];

    do {
        if (!alive_) {
            break;
        }

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

    source_block_resized_ = false;
    repair_block_resized_ = false;
    payload_resized_ = false;

    can_repair_ = false;

    fill_block_();
}

void Reader::try_repair_() {
    if (!can_repair_) {
        return;
    }

    if (!source_block_resized_ || !repair_block_resized_ || !payload_resized_) {
        return;
    }

    if (!decoder_.begin(source_block_.size(), repair_block_.size(), payload_size_)) {
        roc_log(LogDebug,
                "fec reader: can't begin decoder block, shutting down:"
                " sbl=%lu rbl=%lu payload_size=%lu",
                (unsigned long)source_block_.size(), (unsigned long)repair_block_.size(),
                (unsigned long)payload_size_);
        alive_ = false;
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

        packet::PacketPtr pp = parse_repaired_packet_(buffer);
        if (!pp) {
            continue;
        }

        source_block_[n] = pp;
    }

    decoder_.end();
    can_repair_ = false;
}

packet::PacketPtr Reader::parse_repaired_packet_(const core::Slice<uint8_t>& buffer) {
    packet::PacketPtr pp = new (packet_pool_) packet::Packet(packet_pool_);
    if (!pp) {
        roc_log(LogError, "fec reader: can't allocate packet");
        return NULL;
    }

    if (!parser_.parse(*pp, buffer)) {
        roc_log(LogDebug, "fec reader: can't parse repaired packet");
        return NULL;
    }

    pp->set_data(buffer);
    pp->add_flags(packet::Packet::FlagRestored);

    return pp;
}

void Reader::fetch_packets_() {
    for (;;) {
        if (packet::PacketPtr pp = source_reader_.read()) {
            if (!validate_fec_packet_(pp)) {
                return;
            }
            source_queue_.write(pp);
        } else {
            break;
        }
    }

    for (;;) {
        if (packet::PacketPtr pp = repair_reader_.read()) {
            if (!validate_fec_packet_(pp)) {
                return;
            }
            repair_queue_.write(pp);
        } else {
            break;
        }
    }
}

void Reader::fill_block_() {
    fill_source_block_();
    fill_repair_block_();
}

void Reader::fill_source_block_() {
    unsigned n_fetched = 0, n_added = 0, n_dropped = 0;

    for (;;) {
        packet::PacketPtr pp = source_queue_.head();
        if (!pp) {
            break;
        }

        if (!validate_sbn_sequence_(pp)) {
            break;
        }

        const packet::FEC& fec = *pp->fec();

        if (!packet::blknum_le(fec.source_block_number, cur_sbn_)) {
            break;
        }

        (void)source_queue_.read();
        n_fetched++;

        if (packet::blknum_lt(fec.source_block_number, cur_sbn_)) {
            roc_log(LogTrace,
                    "fec reader: dropping source packet from previous block:"
                    " cur_sbn=%lu pkt_sbn=%lu pkt_esi=%lu",
                    (unsigned long)cur_sbn_, (unsigned long)fec.source_block_number,
                    (unsigned long)fec.encoding_symbol_id);
            n_dropped++;
            continue;
        }

        // should not happen: we have handled preceding and following blocks above
        roc_panic_if_not(fec.source_block_number == cur_sbn_);

        if (!process_source_packet_(pp)) {
            roc_log(LogTrace,
                    "fec reader: dropping source packet from current block:"
                    " esi=%lu sblen=%lu blen=%lu payload_size=%lu",
                    (unsigned long)fec.encoding_symbol_id,
                    (unsigned long)fec.source_block_length,
                    (unsigned long)fec.block_length, (unsigned long)fec.payload.size());
            n_dropped++;
            continue;
        }

        // should not happen: we have handled validation and block size above
        roc_panic_if_not(fec.source_block_length == source_block_.size());
        roc_panic_if_not(fec.encoding_symbol_id < source_block_.size());

        const size_t p_num = fec.encoding_symbol_id;

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

void Reader::fill_repair_block_() {
    unsigned n_fetched = 0, n_added = 0, n_dropped = 0;

    for (;;) {
        packet::PacketPtr pp = repair_queue_.head();
        if (!pp) {
            break;
        }

        if (!validate_sbn_sequence_(pp)) {
            break;
        }

        const packet::FEC& fec = *pp->fec();

        if (!packet::blknum_le(fec.source_block_number, cur_sbn_)) {
            break;
        }

        (void)repair_queue_.read();
        n_fetched++;

        if (packet::blknum_lt(fec.source_block_number, cur_sbn_)) {
            roc_log(LogTrace,
                    "fec reader: dropping repair packet from previous block:"
                    " cur_sbn=%lu pkt_sbn=%lu",
                    (unsigned long)cur_sbn_, (unsigned long)fec.source_block_number);
            n_dropped++;
            continue;
        }

        // should not happen: we have handled preceding and following blocks above
        roc_panic_if(fec.source_block_number != cur_sbn_);

        if (!process_repair_packet_(pp)) {
            roc_log(LogTrace,
                    "fec reader: dropping repair packet from current block:"
                    " esi=%lu sblen=%lu blen=%lu payload_size=%lu",
                    (unsigned long)fec.encoding_symbol_id,
                    (unsigned long)fec.source_block_length,
                    (unsigned long)fec.block_length, (unsigned long)fec.payload.size());
            n_dropped++;
            continue;
        }

        // should not happen: we have handled validation and block size above
        roc_panic_if_not(fec.source_block_length == source_block_.size());
        roc_panic_if_not(fec.encoding_symbol_id >= source_block_.size());
        roc_panic_if_not(fec.encoding_symbol_id
                         < source_block_.size() + repair_block_.size());

        const size_t p_num = fec.encoding_symbol_id - fec.source_block_length;

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

bool Reader::process_source_packet_(const packet::PacketPtr& pp) {
    const packet::FEC& fec = *pp->fec();

    if (!validate_incoming_source_packet_(pp)) {
        return false;
    }

    if (!can_update_payload_size_(fec.payload.size())) {
        return false;
    }

    if (!can_update_source_block_size_(fec.source_block_length)) {
        return false;
    }

    if (!update_payload_size_(fec.payload.size())) {
        return false;
    }

    if (!update_source_block_size_(fec.source_block_length)) {
        return false;
    }

    return true;
}

bool Reader::process_repair_packet_(const packet::PacketPtr& pp) {
    const packet::FEC& fec = *pp->fec();

    if (!validate_incoming_repair_packet_(pp)) {
        return false;
    }

    if (!can_update_payload_size_(fec.payload.size())) {
        return false;
    }

    if (!can_update_source_block_size_(fec.source_block_length)) {
        return false;
    }

    if (!can_update_repair_block_size_(fec.block_length)) {
        return false;
    }

    if (!update_payload_size_(fec.payload.size())) {
        return false;
    }

    if (!update_source_block_size_(fec.source_block_length)) {
        return false;
    }

    if (!update_repair_block_size_(fec.block_length)) {
        return false;
    }

    return true;
}

bool Reader::validate_fec_packet_(const packet::PacketPtr& pp) {
    const packet::FEC* fec = pp->fec();

    if (!fec) {
        roc_panic("fec reader: unexpected non-fec source packet");
    }

    if (fec->fec_scheme != fec_scheme_) {
        roc_log(LogDebug,
                "fec reader: unexpected packet fec scheme, shutting down:"
                " packet_scheme=%s session_scheme=%s",
                packet::fec_scheme_to_str(fec->fec_scheme),
                packet::fec_scheme_to_str(fec_scheme_));
        return (alive_ = false);
    }

    return true;
}

bool Reader::validate_sbn_sequence_(const packet::PacketPtr& pp) {
    const packet::FEC& fec = *pp->fec();

    packet::blknum_diff_t blk_dist =
        packet::blknum_diff(fec.source_block_number, cur_sbn_);

    if (blk_dist < 0) {
        blk_dist = -blk_dist;
    }

    if ((size_t)blk_dist > max_sbn_jump_) {
        roc_log(LogDebug,
                "fec reader: too long source block number jump, shutting down:"
                " cur_sbn=%lu pkt_sbn=%lu dist=%lu max=%lu",
                (unsigned long)cur_sbn_, (unsigned long)fec.source_block_number,
                (unsigned long)blk_dist, (unsigned long)max_sbn_jump_);
        return (alive_ = false);
    }

    return true;
}

bool Reader::validate_incoming_source_packet_(const packet::PacketPtr& pp) {
    const packet::FEC& fec = *pp->fec();

    if (!(fec.encoding_symbol_id < fec.source_block_length)) {
        return false;
    }

    if (fec.source_block_length == 0) {
        return false;
    }

    if (fec.block_length != 0) {
        if (!(fec.source_block_length <= fec.block_length)) {
            return false;
        }
    }

    if (fec.payload.size() == 0) {
        return false;
    }

    return true;
}

bool Reader::validate_incoming_repair_packet_(const packet::PacketPtr& pp) {
    const packet::FEC& fec = *pp->fec();

    if (!(fec.encoding_symbol_id >= fec.source_block_length)) {
        return false;
    }

    if (fec.source_block_length == 0) {
        return false;
    }

    if (fec.block_length != 0) {
        if (!(fec.encoding_symbol_id < fec.block_length)) {
            return false;
        }

        if (!(fec.source_block_length <= fec.block_length)) {
            return false;
        }
    }

    if (fec.payload.size() == 0) {
        return false;
    }

    return true;
}

bool Reader::can_update_payload_size_(size_t new_payload_size) {
    if (payload_size_ == new_payload_size) {
        return true;
    }

    if (payload_resized_) {
        roc_log(LogDebug,
                "fec reader: can't change payload size in the middle of a block:"
                " next_esi=%lu cur_size=%lu new_size=%lu",
                (unsigned long)next_packet_, (unsigned long)payload_size_,
                (unsigned long)new_payload_size);
        return false;
    }

    return true;
}

bool Reader::update_payload_size_(size_t new_payload_size) {
    if (payload_size_ == new_payload_size) {
        payload_resized_ = true;
        return true;
    }

    roc_log(LogDebug,
            "fec reader: update payload size: next_esi=%lu cur_size=%lu new_size=%lu",
            (unsigned long)next_packet_, (unsigned long)payload_size_,
            (unsigned long)new_payload_size);

    payload_size_ = new_payload_size;
    payload_resized_ = true;

    return true;
}

bool Reader::can_update_source_block_size_(size_t new_sblen) {
    const size_t cur_sblen = source_block_.size();

    if (cur_sblen == new_sblen) {
        return true;
    }

    if (source_block_resized_) {
        roc_log(LogDebug,
                "fec reader: can't change source block size in the middle of a block:"
                " next_esi=%lu cur_sblen=%lu new_sblen=%lu",
                (unsigned long)next_packet_, (unsigned long)cur_sblen,
                (unsigned long)new_sblen);
        return false;
    }

    if (new_sblen > decoder_.max_block_length()) {
        roc_log(LogDebug,
                "fec reader: can't change source block size above maximum, shutting down:"
                " cur_sblen=%lu new_sblen=%lu max_blen=%lu",
                (unsigned long)cur_sblen, (unsigned long)new_sblen,
                (unsigned long)decoder_.max_block_length());
        return (alive_ = false);
    }

    return true;
}

bool Reader::update_source_block_size_(size_t new_sblen) {
    const size_t cur_sblen = source_block_.size();

    if (cur_sblen == new_sblen) {
        source_block_resized_ = true;
        return true;
    }

    if (!source_block_.resize(new_sblen)) {
        roc_log(LogDebug,
                "fec reader: can't allocate source block memory, shutting down:"
                " cur_sblen=%lu new_sblen=%lu",
                (unsigned long)cur_sblen, (unsigned long)new_sblen);
        return (alive_ = false);
    }

    roc_log(LogDebug,
            "fec reader: update source block size:"
            " cur_sblen=%lu cur_rblen=%lu new_sblen=%lu",
            (unsigned long)cur_sblen, (unsigned long)repair_block_.size(),
            (unsigned long)new_sblen);

    source_block_resized_ = true;

    return true;
}

bool Reader::can_update_repair_block_size_(size_t new_blen) {
    const size_t cur_blen = source_block_.size() + repair_block_.size();

    if (new_blen == cur_blen) {
        return true;
    }

    if (repair_block_resized_) {
        roc_log(LogDebug,
                "fec reader: can't change repair block size in the middle of a block:"
                " next_esi=%lu cur_blen=%lu new_blen=%lu",
                (unsigned long)next_packet_, (unsigned long)cur_blen,
                (unsigned long)new_blen);
        return false;
    }

    if (new_blen > decoder_.max_block_length()) {
        roc_log(LogDebug,
                "fec reader: can't change repair block size above maximum, shutting down:"
                " cur_blen=%lu new_blen=%lu max_blen=%lu",
                (unsigned long)cur_blen, (unsigned long)new_blen,
                (unsigned long)decoder_.max_block_length());
        return (alive_ = false);
    }

    return true;
}

bool Reader::update_repair_block_size_(size_t new_blen) {
    const size_t cur_sblen = source_block_.size();
    const size_t cur_rblen = repair_block_.size();

    const size_t cur_blen = cur_sblen + cur_rblen;

    if (new_blen == cur_blen) {
        repair_block_resized_ = true;
        return true;
    }

    // shoud not happen: sblen should be validated and updated already
    roc_panic_if_not(new_blen > cur_sblen);

    const size_t new_rblen = new_blen - cur_sblen;

    if (!repair_block_.resize(new_rblen)) {
        roc_log(LogDebug,
                "fec reader: can't allocate repair block memory, shutting down:"
                " cur_rblen=%lu new_rblen=%lu",
                (unsigned long)cur_rblen, (unsigned long)new_rblen);
        return (alive_ = false);
    }

    roc_log(LogDebug,
            "fec reader: update repair block size:"
            " cur_sblen=%lu cur_rblen=%lu new_rblen=%lu",
            (unsigned long)cur_sblen, (unsigned long)cur_rblen, (unsigned long)new_rblen);

    repair_block_resized_ = true;

    return true;
}

void Reader::drop_repair_packets_from_prev_blocks_() {
    unsigned n_dropped = 0;

    for (;;) {
        packet::PacketPtr pp = repair_queue_.head();
        if (!pp) {
            break;
        }

        const packet::FEC& fec = *pp->fec();

        if (!packet::blknum_lt(fec.source_block_number, cur_sbn_)) {
            break;
        }

        roc_log(LogTrace,
                "fec reader: dropping repair packet from previous blocks,"
                " decoding not started: cur_sbn=%lu pkt_sbn=%lu",
                (unsigned long)cur_sbn_, (unsigned long)fec.source_block_number);

        (void)repair_queue_.read();
        n_dropped++;
    }

    if (n_dropped != 0) {
        roc_log(LogDebug, "fec reader: repair queue: dropped=%u", n_dropped);
    }
}

} // namespace fec
} // namespace roc
