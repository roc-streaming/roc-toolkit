/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_fec/reader.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_packet/fec_scheme_to_str.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace fec {

Reader::Reader(const ReaderConfig& config,
               packet::FecScheme fec_scheme,
               IBlockDecoder& block_decoder,
               packet::IReader& source_reader,
               packet::IReader& repair_reader,
               packet::IParser& parser,
               packet::PacketFactory& packet_factory,
               core::IArena& arena)
    : block_decoder_(block_decoder)
    , source_reader_(source_reader)
    , repair_reader_(repair_reader)
    , parser_(parser)
    , packet_factory_(packet_factory)
    , source_queue_(0)
    , repair_queue_(0)
    , source_block_(arena)
    , repair_block_(arena)
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
    , prev_block_timestamp_valid_(false)
    , block_max_duration_(0)
    , max_sbn_jump_(config.max_sbn_jump)
    , fec_scheme_(fec_scheme) {
    valid_ = true;
}

bool Reader::is_valid() const {
    return valid_;
}

bool Reader::is_started() const {
    return started_;
}

bool Reader::is_alive() const {
    return alive_;
}

status::StatusCode Reader::read(packet::PacketPtr& pp) {
    roc_panic_if_not(is_valid());

    if (!alive_) {
        // TODO(gh-183): return StatusDead
        return status::StatusNoData;
    }

    status::StatusCode code = read_(pp);
    if (code == status::StatusOK) {
        n_packets_++;
    }
    if (!alive_) {
        pp = NULL;
        // TODO(gh-183): return StatusDead
        return status::StatusNoData;
    }

    return code;
}

status::StatusCode Reader::read_(packet::PacketPtr& ptr) {
    const status::StatusCode code = fetch_all_packets_();
    if (code != status::StatusOK) {
        return code;
    }

    if (!started_) {
        started_ = try_start_();
    }

    if (!started_) {
        // until started, just forward all source packets
        return source_queue_.read(ptr);
    }

    return get_next_packet_(ptr);
}

bool Reader::try_start_() {
    packet::PacketPtr pp = source_queue_.head();
    if (!pp) {
        return false;
    }

    const packet::FEC& fec = *pp->fec();

    if (!process_source_packet_(pp)) {
        roc_log(LogTrace,
                "fec reader: dropping leading source packet:"
                " esi=%lu sblen=%lu blen=%lu payload_size=%lu",
                (unsigned long)fec.encoding_symbol_id,
                (unsigned long)fec.source_block_length, (unsigned long)fec.block_length,
                (unsigned long)fec.payload.size());
        return false;
    }

    cur_sbn_ = fec.source_block_number;
    drop_repair_packets_from_prev_blocks_();

    if (pp->fec()->encoding_symbol_id > 0) {
        // Wait until we receive first packet in block (ESI=0), see also gh-186.
        return false;
    }

    roc_log(LogDebug,
            "fec reader: got first packet in a block, start decoding:"
            " n_packets_before=%u sbn=%lu",
            n_packets_, (unsigned long)cur_sbn_);

    started_ = true;

    return true;
}

status::StatusCode Reader::get_next_packet_(packet::PacketPtr& ptr) {
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
                    return status::StatusNoData;
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

    ptr = pp;

    return status::StatusOK;
}

void Reader::next_block_() {
    roc_log(LogTrace, "fec reader: next block: sbn=%lu", (unsigned long)cur_sbn_);

    if (source_block_[0]) {
        update_block_duration_(source_block_[0]);
    } else {
        prev_block_timestamp_valid_ = false;
    }

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

    if (!block_decoder_.begin(source_block_.size(), repair_block_.size(),
                              payload_size_)) {
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
        block_decoder_.set(n, source_block_[n]->fec()->payload);
    }

    for (size_t n = 0; n < repair_block_.size(); n++) {
        if (!repair_block_[n]) {
            continue;
        }
        block_decoder_.set(source_block_.size() + n, repair_block_[n]->fec()->payload);
    }

    for (size_t n = 0; n < source_block_.size(); n++) {
        if (source_block_[n]) {
            continue;
        }

        core::Slice<uint8_t> buffer = block_decoder_.repair(n);
        if (!buffer) {
            continue;
        }

        packet::PacketPtr pp = parse_repaired_packet_(buffer);
        if (!pp) {
            continue;
        }

        source_block_[n] = pp;
    }

    block_decoder_.end();
    can_repair_ = false;
}

packet::PacketPtr Reader::parse_repaired_packet_(const core::Slice<uint8_t>& buffer) {
    packet::PacketPtr pp = packet_factory_.new_packet();
    if (!pp) {
        roc_log(LogError, "fec reader: can't allocate packet");
        return NULL;
    }

    if (!parser_.parse(*pp, buffer)) {
        roc_log(LogDebug, "fec reader: can't parse repaired packet");
        return NULL;
    }

    pp->set_buffer(buffer);
    pp->add_flags(packet::Packet::FlagRestored);

    return pp;
}

status::StatusCode Reader::fetch_all_packets_() {
    status::StatusCode code = fetch_packets_(source_reader_, source_queue_);
    if (code == status::StatusOK) {
        code = fetch_packets_(repair_reader_, repair_queue_);
    }

    return code;
}

status::StatusCode Reader::fetch_packets_(packet::IReader& reader,
                                          packet::IWriter& writer) {
    for (;;) {
        packet::PacketPtr pp;

        status::StatusCode code = reader.read(pp);
        if (code != status::StatusOK) {
            if (code == status::StatusNoData) {
                break;
            }
            return code;
        }

        if (!validate_fec_packet_(pp)) {
            break;
        }

        code = writer.write(pp);
        // TODO(gh-183): forward status
        roc_panic_if(code != status::StatusOK);
    }

    return status::StatusOK;
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

        packet::PacketPtr p;
        const status::StatusCode code = source_queue_.read(p);
        roc_panic_if_msg(code != status::StatusOK,
                         "failed to read source packet: status=%s",
                         status::code_to_str(code));
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

        packet::PacketPtr p;
        const status::StatusCode code = repair_queue_.read(p);
        roc_panic_if_msg(code != status::StatusOK,
                         "failed to read repair packet: status=%s",
                         status::code_to_str(code));
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

    if (new_sblen > block_decoder_.max_block_length()) {
        roc_log(LogDebug,
                "fec reader: can't change source block size above maximum, shutting down:"
                " cur_sblen=%lu new_sblen=%lu max_blen=%lu",
                (unsigned long)cur_sblen, (unsigned long)new_sblen,
                (unsigned long)block_decoder_.max_block_length());
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

    prev_block_timestamp_valid_ = false;
    block_max_duration_ = 0;

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

    if (new_blen > block_decoder_.max_block_length()) {
        roc_log(LogDebug,
                "fec reader: can't change repair block size above maximum, shutting down:"
                " cur_blen=%lu new_blen=%lu max_blen=%lu",
                (unsigned long)cur_blen, (unsigned long)new_blen,
                (unsigned long)block_decoder_.max_block_length());
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

    prev_block_timestamp_valid_ = false;
    block_max_duration_ = 0;

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

        packet::PacketPtr p;
        const status::StatusCode code = repair_queue_.read(p);
        roc_panic_if_msg(code != status::StatusOK,
                         "failed to read repair packet: status=%s",
                         status::code_to_str(code));
        n_dropped++;
    }

    if (n_dropped != 0) {
        roc_log(LogDebug, "fec reader: repair queue: dropped=%u", n_dropped);
    }
}

void Reader::update_block_duration_(const packet::PacketPtr& ptr) {
    packet::stream_timestamp_diff_t block_dur = 0;
    if (prev_block_timestamp_valid_) {
        block_dur =
            packet::stream_timestamp_diff(ptr->stream_timestamp(), prev_block_timestamp_);
    }
    if (block_dur < 0) {
        roc_log(LogTrace, "fec reader: negative block duration");
        prev_block_timestamp_valid_ = false;
    } else {
        block_max_duration_ = std::max(block_max_duration_, block_dur);
        prev_block_timestamp_ = ptr->stream_timestamp();
        prev_block_timestamp_valid_ = true;
    }
}

packet::stream_timestamp_t Reader::max_block_duration() const {
    return (packet::stream_timestamp_t)block_max_duration_;
}

} // namespace fec
} // namespace roc
