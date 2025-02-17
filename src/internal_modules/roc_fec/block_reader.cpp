/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_fec/block_reader.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_packet/fec_scheme.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace fec {

BlockReader::BlockReader(const BlockReaderConfig& config,
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
    , started_(false)
    , can_repair_(false)
    , head_index_(0)
    , cur_sbn_(0)
    , payload_size_(0)
    , source_block_resized_(false)
    , repair_block_resized_(false)
    , payload_resized_(false)
    , n_packets_(0)
    , prev_block_timestamp_valid_(false)
    , block_max_duration_(0)
    , max_sbn_jump_(config.max_sbn_jump)
    , fec_scheme_(fec_scheme)
    , init_status_(status::NoStatus) {
    if ((init_status_ = block_decoder_.init_status()) != status::StatusOK) {
        return;
    }
    init_status_ = status::StatusOK;
}

status::StatusCode BlockReader::init_status() const {
    return init_status_;
}

bool BlockReader::is_started() const {
    return started_;
}

packet::stream_timestamp_t BlockReader::max_block_duration() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return (packet::stream_timestamp_t)block_max_duration_;
}

status::StatusCode BlockReader::read(packet::PacketPtr& pp, packet::PacketReadMode mode) {
    roc_panic_if(init_status_ != status::StatusOK);

    status::StatusCode code = status::NoStatus;

    // Greedily fetch packets from underlying readers to queues.
    if ((code = fetch_all_packets_()) != status::StatusOK) {
        roc_panic_if(code == status::StatusDrain);
        return code;
    }

    // Try starting until we get first eligible packet.
    if (!started_) {
        if ((code = try_start_()) != status::StatusOK) {
            roc_panic_if(code == status::StatusDrain);
            return code;
        }
    }

    if (!started_) {
        // Until started, just forward all source packets.
        code = source_queue_.read(pp, mode);
    } else {
        // Normal read.
        code = get_next_packet_(pp, mode);
    }

    if (code == status::StatusOK && mode == packet::ModeFetch) {
        n_packets_++;
    }
    return code;
}

status::StatusCode BlockReader::try_start_() {
    packet::PacketPtr pp = source_queue_.head();
    if (!pp) {
        return status::StatusOK;
    }

    const packet::FEC& fec = *pp->fec();

    const status::StatusCode code = process_source_packet_(pp);
    if (code == status::StatusBadPacket) {
        // Wait until we receive a valid packet.
        roc_log(LogTrace,
                "fec block reader: skipping leading source packet:"
                " esi=%lu sblen=%lu blen=%lu payload_size=%lu",
                (unsigned long)fec.encoding_symbol_id,
                (unsigned long)fec.source_block_length, (unsigned long)fec.block_length,
                (unsigned long)fec.payload.size());
        return status::StatusOK;
    }
    if (code != status::StatusOK) {
        // Unexpected failure, aborting.
        return code;
    }

    cur_sbn_ = fec.source_block_number;
    drop_repair_packets_from_prev_blocks_();

    if (fec.encoding_symbol_id > 0) {
        // Wait until we receive first packet in block (ESI=0).
        // See also gh-186.
        roc_log(LogTrace,
                "fec block reader: skipping leading source packet:"
                " esi=%lu sblen=%lu blen=%lu payload_size=%lu",
                (unsigned long)fec.encoding_symbol_id,
                (unsigned long)fec.source_block_length, (unsigned long)fec.block_length,
                (unsigned long)fec.payload.size());
        return status::StatusOK;
    }

    roc_log(LogDebug,
            "fec block reader: got first packet in a block, start decoding:"
            " n_packets_before=%u sbn=%lu",
            n_packets_, (unsigned long)cur_sbn_);

    started_ = true;

    return status::StatusOK;
}

status::StatusCode BlockReader::get_next_packet_(packet::PacketPtr& result_pkt,
                                                 packet::PacketReadMode mode) {
    const status::StatusCode code = fill_block_();
    if (code != status::StatusOK) {
        roc_panic_if(code == status::StatusDrain);
        return code;
    }

    packet::PacketPtr pkt = source_block_[head_index_];

    for (;;) {
        size_t next_index = 0;

        if (pkt) {
            next_index = head_index_ + 1;
        } else {
            // Try repairing as much as possible and store in block.
            const status::StatusCode code = try_repair_();
            if (code != status::StatusOK) {
                roc_panic_if(code == status::StatusDrain);
                return code;
            }

            // Find first present packet in block, starting from head.
            for (next_index = head_index_; next_index < source_block_.size();
                 next_index++) {
                if (source_block_[next_index]) {
                    pkt = source_block_[next_index];
                    next_index++;
                    break;
                }
            }
        }

        if (!pkt && source_queue_.size() == 0) {
            // No head packet, no queued packets, give up.
            break;
        }
        if (mode == packet::ModePeek) {
            // In peek mode, we just return what we've found, but don't move forward.
            // We could do a better job if we were decoding two blocks simultaneously:
            // current block and next block, to be able to use next block for ModePeek.
            // However, this would significantly complicate implementation.
            break;
        }

        // Switch to next packet and maybe next block.
        head_index_ = next_index;
        if (head_index_ == source_block_.size()) {
            const status::StatusCode code = next_block_();
            if (code != status::StatusOK) {
                roc_panic_if(code == status::StatusDrain);
                return code;
            }
        }

        if (pkt) { // Found packet.
            break;
        }
    }

    result_pkt = pkt;
    return pkt ? status::StatusOK : status::StatusDrain;
}

status::StatusCode BlockReader::next_block_() {
    roc_log(LogTrace, "fec block reader: next block: sbn=%lu", (unsigned long)cur_sbn_);

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
    head_index_ = 0;

    source_block_resized_ = false;
    repair_block_resized_ = false;
    payload_resized_ = false;

    can_repair_ = false;

    return fill_block_();
}

status::StatusCode BlockReader::try_repair_() {
    const bool is_block_resized =
        source_block_resized_ && repair_block_resized_ && payload_resized_;

    if (!can_repair_ || !is_block_resized) {
        // Can't do anything right now.
        return status::StatusOK;
    }

    const status::StatusCode code = block_decoder_.begin_block(
        source_block_.size(), repair_block_.size(), payload_size_);

    if (code != status::StatusOK) {
        roc_log(LogError,
                "fec block reader: can't begin decoder block:"
                " sbl=%lu rbl=%lu payload_size=%lu",
                (unsigned long)source_block_.size(), (unsigned long)repair_block_.size(),
                (unsigned long)payload_size_);
        return code;
    }

    for (size_t n = 0; n < source_block_.size(); n++) {
        if (!source_block_[n]) {
            continue;
        }
        block_decoder_.set_buffer(n, source_block_[n]->fec()->payload);
    }

    for (size_t n = 0; n < repair_block_.size(); n++) {
        if (!repair_block_[n]) {
            continue;
        }
        block_decoder_.set_buffer(source_block_.size() + n,
                                  repair_block_[n]->fec()->payload);
    }

    for (size_t n = 0; n < source_block_.size(); n++) {
        if (source_block_[n]) {
            continue;
        }

        core::Slice<uint8_t> buffer = block_decoder_.repair_buffer(n);
        if (!buffer) {
            continue;
        }

        packet::PacketPtr packet;
        const status::StatusCode code = parse_repaired_packet_(buffer, packet);
        if (code == status::StatusBadPacket) {
            continue;
        }
        if (code != status::StatusOK) {
            return code;
        }

        source_block_[n] = packet;
    }

    block_decoder_.end_block();
    can_repair_ = false;

    return status::StatusOK;
}

status::StatusCode BlockReader::parse_repaired_packet_(const core::Slice<uint8_t>& buffer,
                                                       packet::PacketPtr& result_packet) {
    packet::PacketPtr pp = packet_factory_.new_packet();
    if (!pp) {
        roc_log(LogError, "fec block reader: can't allocate packet");
        return status::StatusNoMem;
    }

    if (!parser_.parse(*pp, buffer)) {
        roc_log(LogDebug, "fec block reader: can't parse repaired packet");
        // Upper code expects StatusBadPacket in this case.
        return status::StatusBadPacket;
    }

    pp->set_buffer(buffer);
    pp->add_flags(packet::Packet::FlagRestored);

    result_packet = pp;

    return status::StatusOK;
}

status::StatusCode BlockReader::fetch_all_packets_() {
    status::StatusCode code = status::NoStatus;

    if ((code = fetch_packets_(source_reader_, source_queue_)) != status::StatusOK) {
        return code;
    }

    if ((code = fetch_packets_(repair_reader_, repair_queue_)) != status::StatusOK) {
        return code;
    }

    return status::StatusOK;
}

status::StatusCode BlockReader::fetch_packets_(packet::IReader& reader,
                                               packet::IWriter& writer) {
    for (;;) {
        packet::PacketPtr pp;

        status::StatusCode code = reader.read(pp, packet::ModeFetch);
        if (code == status::StatusDrain) {
            // Fine, no more packets right now.
            break;
        }
        if (code != status::StatusOK) {
            return code;
        }

        if (!validate_fec_packet_(pp)) {
            return status::StatusAbort;
        }

        code = writer.write(pp);
        if (code != status::StatusOK) {
            return code;
        }
    }

    return status::StatusOK;
}

status::StatusCode BlockReader::fill_block_() {
    status::StatusCode code = status::NoStatus;

    if ((code = fill_source_block_()) != status::StatusOK) {
        return code;
    }

    if ((code = fill_repair_block_()) != status::StatusOK) {
        return code;
    }

    return status::StatusOK;
}

status::StatusCode BlockReader::fill_source_block_() {
    unsigned n_fetched = 0, n_added = 0, n_dropped = 0;

    for (;;) {
        packet::PacketPtr pp = source_queue_.head();
        if (!pp) {
            break;
        }

        if (!validate_sbn_sequence_(pp)) {
            return status::StatusAbort;
        }

        const packet::FEC& fec = *pp->fec();

        if (!packet::blknum_le(fec.source_block_number, cur_sbn_)) {
            break;
        }

        packet::PacketPtr p;
        status::StatusCode code = source_queue_.read(p, packet::ModeFetch);
        roc_panic_if_msg(code != status::StatusOK,
                         "failed to read source packet: status=%s",
                         status::code_to_str(code));
        n_fetched++;

        if (packet::blknum_lt(fec.source_block_number, cur_sbn_)) {
            roc_log(LogTrace,
                    "fec block reader: dropping source packet from previous block:"
                    " cur_sbn=%lu pkt_sbn=%lu pkt_esi=%lu",
                    (unsigned long)cur_sbn_, (unsigned long)fec.source_block_number,
                    (unsigned long)fec.encoding_symbol_id);
            n_dropped++;
            continue;
        }

        // Should not happen: we have handled preceding and following blocks above.
        roc_panic_if_not(fec.source_block_number == cur_sbn_);

        code = process_source_packet_(pp);
        if (code == status::StatusBadPacket) {
            roc_log(LogTrace,
                    "fec block reader: dropping source packet from current block:"
                    " esi=%lu sblen=%lu blen=%lu payload_size=%lu",
                    (unsigned long)fec.encoding_symbol_id,
                    (unsigned long)fec.source_block_length,
                    (unsigned long)fec.block_length, (unsigned long)fec.payload.size());
            n_dropped++;
            continue;
        }
        if (code != status::StatusOK) {
            // Unexpected failure, aborting.
            return code;
        }

        // Should not happen: we have handled validation and block size above.
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
        roc_log(LogDebug,
                "fec block reader: source queue: fetched=%u added=%u dropped=%u",
                n_fetched, n_added, n_dropped);
    }

    return status::StatusOK;
}

status::StatusCode BlockReader::fill_repair_block_() {
    unsigned n_fetched = 0, n_added = 0, n_dropped = 0;

    for (;;) {
        packet::PacketPtr pp = repair_queue_.head();
        if (!pp) {
            break;
        }

        if (!validate_sbn_sequence_(pp)) {
            return status::StatusAbort;
        }

        const packet::FEC& fec = *pp->fec();

        if (!packet::blknum_le(fec.source_block_number, cur_sbn_)) {
            break;
        }

        packet::PacketPtr p;
        status::StatusCode code = repair_queue_.read(p, packet::ModeFetch);
        roc_panic_if_msg(code != status::StatusOK,
                         "failed to read repair packet: status=%s",
                         status::code_to_str(code));
        n_fetched++;

        if (packet::blknum_lt(fec.source_block_number, cur_sbn_)) {
            roc_log(LogTrace,
                    "fec block reader: dropping repair packet from previous block:"
                    " cur_sbn=%lu pkt_sbn=%lu",
                    (unsigned long)cur_sbn_, (unsigned long)fec.source_block_number);
            n_dropped++;
            continue;
        }

        // Should not happen: we have handled preceding and following blocks above.
        roc_panic_if(fec.source_block_number != cur_sbn_);

        code = process_repair_packet_(pp);
        if (code == status::StatusBadPacket) {
            roc_log(LogTrace,
                    "fec block reader: dropping repair packet from current block:"
                    " esi=%lu sblen=%lu blen=%lu payload_size=%lu",
                    (unsigned long)fec.encoding_symbol_id,
                    (unsigned long)fec.source_block_length,
                    (unsigned long)fec.block_length, (unsigned long)fec.payload.size());
            n_dropped++;
            continue;
        }
        if (code != status::StatusOK) {
            // Unexpected failure, aborting.
            return code;
        }

        // Should not happen: we have handled validation and block size above.
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
        roc_log(LogDebug,
                "fec block reader: repair queue: fetched=%u added=%u dropped=%u",
                n_fetched, n_added, n_dropped);
    }

    return status::StatusOK;
}

status::StatusCode BlockReader::process_source_packet_(const packet::PacketPtr& pp) {
    status::StatusCode code = status::NoStatus;

    const packet::FEC& fec = *pp->fec();

    if (!validate_incoming_source_packet_(pp)) {
        return status::StatusBadPacket;
    }

    if (!can_update_payload_size_(fec.payload.size())) {
        return status::StatusBadPacket;
    }

    if (!can_update_source_block_size_(fec.source_block_length)) {
        return status::StatusBadPacket;
    }

    if ((code = update_payload_size_(fec.payload.size())) != status::StatusOK) {
        return code;
    }

    if ((code = update_source_block_size_(fec.source_block_length)) != status::StatusOK) {
        return code;
    }

    return status::StatusOK;
}

status::StatusCode BlockReader::process_repair_packet_(const packet::PacketPtr& pp) {
    status::StatusCode code = status::NoStatus;

    const packet::FEC& fec = *pp->fec();

    if (!validate_incoming_repair_packet_(pp)) {
        return status::StatusBadPacket;
    }

    if (!can_update_payload_size_(fec.payload.size())) {
        return status::StatusBadPacket;
    }

    if (!can_update_source_block_size_(fec.source_block_length)) {
        return status::StatusBadPacket;
    }

    if (!can_update_repair_block_size_(fec.block_length)) {
        return status::StatusBadPacket;
    }

    if ((code = update_payload_size_(fec.payload.size())) != status::StatusOK) {
        return code;
    }

    if ((code = update_source_block_size_(fec.source_block_length)) != status::StatusOK) {
        return code;
    }

    if ((code = update_repair_block_size_(fec.block_length)) != status::StatusOK) {
        return code;
    }

    return status::StatusOK;
}

bool BlockReader::validate_fec_packet_(const packet::PacketPtr& pp) {
    if (!pp->has_flags(packet::Packet::FlagFEC)) {
        roc_panic("fec block reader: unexpected non-fec packet");
    }

    if (pp->fec()->fec_scheme != fec_scheme_) {
        roc_log(LogDebug,
                "fec block reader: unexpected packet fec scheme:"
                " packet_scheme=%s session_scheme=%s",
                packet::fec_scheme_to_str(pp->fec()->fec_scheme),
                packet::fec_scheme_to_str(fec_scheme_));
        return false;
    }

    return true;
}

bool BlockReader::validate_sbn_sequence_(const packet::PacketPtr& pp) {
    const packet::FEC& fec = *pp->fec();

    packet::blknum_diff_t blk_dist =
        packet::blknum_diff(fec.source_block_number, cur_sbn_);

    if (blk_dist < 0) {
        blk_dist = -blk_dist;
    }

    if ((size_t)blk_dist > max_sbn_jump_) {
        roc_log(LogDebug,
                "fec block reader: too long source block number jump:"
                " cur_sbn=%lu pkt_sbn=%lu dist=%lu max=%lu",
                (unsigned long)cur_sbn_, (unsigned long)fec.source_block_number,
                (unsigned long)blk_dist, (unsigned long)max_sbn_jump_);
        return false;
    }

    return true;
}

bool BlockReader::validate_incoming_source_packet_(const packet::PacketPtr& pp) {
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

bool BlockReader::validate_incoming_repair_packet_(const packet::PacketPtr& pp) {
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

bool BlockReader::can_update_payload_size_(size_t new_payload_size) {
    if (payload_size_ == new_payload_size) {
        return true;
    }

    if (payload_resized_) {
        roc_log(LogDebug,
                "fec block reader: can't change payload size in the middle of a block:"
                " next_esi=%lu cur_size=%lu new_size=%lu",
                (unsigned long)head_index_, (unsigned long)payload_size_,
                (unsigned long)new_payload_size);
        return false;
    }

    return true;
}

status::StatusCode BlockReader::update_payload_size_(size_t new_payload_size) {
    if (payload_size_ == new_payload_size) {
        payload_resized_ = true;
        return status::StatusOK;
    }

    roc_log(
        LogDebug,
        "fec block reader: update payload size: next_esi=%lu cur_size=%lu new_size=%lu",
        (unsigned long)head_index_, (unsigned long)payload_size_,
        (unsigned long)new_payload_size);

    payload_size_ = new_payload_size;
    payload_resized_ = true;

    return status::StatusOK;
}

bool BlockReader::can_update_source_block_size_(size_t new_sblen) {
    const size_t cur_sblen = source_block_.size();

    if (cur_sblen == new_sblen) {
        return true;
    }

    if (source_block_resized_) {
        roc_log(
            LogDebug,
            "fec block reader: can't change source block size in the middle of a block:"
            " next_esi=%lu cur_sblen=%lu new_sblen=%lu",
            (unsigned long)head_index_, (unsigned long)cur_sblen,
            (unsigned long)new_sblen);
        return false;
    }

    return true;
}

status::StatusCode BlockReader::update_source_block_size_(size_t new_sblen) {
    const size_t cur_sblen = source_block_.size();

    if (cur_sblen == new_sblen) {
        source_block_resized_ = true;
        return status::StatusOK;
    }

    if (new_sblen > block_decoder_.max_block_length()) {
        roc_log(LogDebug,
                "fec block reader: can't change source block size above maximum:"
                " cur_sblen=%lu new_sblen=%lu max_blen=%lu",
                (unsigned long)cur_sblen, (unsigned long)new_sblen,
                (unsigned long)block_decoder_.max_block_length());
        return status::StatusAbort;
    }

    if (!source_block_.resize(new_sblen)) {
        roc_log(LogError,
                "fec block reader: can't allocate source block memory:"
                " cur_sblen=%lu new_sblen=%lu",
                (unsigned long)cur_sblen, (unsigned long)new_sblen);
        return status::StatusNoMem;
    }

    roc_log(LogDebug,
            "fec block reader: updated source block size:"
            " cur_sblen=%lu cur_rblen=%lu new_sblen=%lu",
            (unsigned long)cur_sblen, (unsigned long)repair_block_.size(),
            (unsigned long)new_sblen);

    // max_block_duration() reports maximum duration since last resize,
    // so when resize happens, we reset maximum.
    prev_block_timestamp_valid_ = false;
    block_max_duration_ = 0;

    source_block_resized_ = true;

    return status::StatusOK;
}

bool BlockReader::can_update_repair_block_size_(size_t new_blen) {
    const size_t cur_blen = source_block_.size() + repair_block_.size();

    if (new_blen == cur_blen) {
        return true;
    }

    if (repair_block_resized_) {
        roc_log(
            LogDebug,
            "fec block reader: can't change repair block size in the middle of a block:"
            " next_esi=%lu cur_blen=%lu new_blen=%lu",
            (unsigned long)head_index_, (unsigned long)cur_blen, (unsigned long)new_blen);
        return false;
    }

    return true;
}

status::StatusCode BlockReader::update_repair_block_size_(size_t new_blen) {
    const size_t cur_sblen = source_block_.size();
    const size_t cur_rblen = repair_block_.size();

    const size_t cur_blen = cur_sblen + cur_rblen;

    if (new_blen == cur_blen) {
        repair_block_resized_ = true;
        return status::StatusOK;
    }

    if (new_blen > block_decoder_.max_block_length()) {
        roc_log(LogDebug,
                "fec block reader: can't change repair block size above maximum:"
                " cur_blen=%lu new_blen=%lu max_blen=%lu",
                (unsigned long)cur_blen, (unsigned long)new_blen,
                (unsigned long)block_decoder_.max_block_length());
        return status::StatusAbort;
    }

    // Should not happen: sblen should be validated in upper code.
    roc_panic_if_not(new_blen > cur_sblen);

    const size_t new_rblen = new_blen - cur_sblen;

    if (!repair_block_.resize(new_rblen)) {
        roc_log(LogError,
                "fec block reader: can't allocate repair block memory:"
                " cur_rblen=%lu new_rblen=%lu",
                (unsigned long)cur_rblen, (unsigned long)new_rblen);
        return status::StatusNoMem;
    }

    roc_log(LogDebug,
            "fec block reader: updated repair block size:"
            " cur_sblen=%lu cur_rblen=%lu new_rblen=%lu",
            (unsigned long)cur_sblen, (unsigned long)cur_rblen, (unsigned long)new_rblen);

    // max_block_duration() reports maximum duration since last resize,
    // so when resize happens, we reset maximum.
    prev_block_timestamp_valid_ = false;
    block_max_duration_ = 0;

    repair_block_resized_ = true;

    return status::StatusOK;
}

void BlockReader::drop_repair_packets_from_prev_blocks_() {
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
                "fec block reader: dropping repair packet from previous blocks,"
                " decoding not started: cur_sbn=%lu pkt_sbn=%lu",
                (unsigned long)cur_sbn_, (unsigned long)fec.source_block_number);

        packet::PacketPtr p;
        const status::StatusCode code = repair_queue_.read(p, packet::ModeFetch);
        roc_panic_if_msg(code != status::StatusOK,
                         "failed to read repair packet: status=%s",
                         status::code_to_str(code));
        n_dropped++;
    }

    if (n_dropped != 0) {
        roc_log(LogDebug, "fec block reader: repair queue: dropped=%u", n_dropped);
    }
}

void BlockReader::update_block_duration_(const packet::PacketPtr& curr_block_pkt) {
    packet::stream_timestamp_diff_t block_dur = 0;
    if (prev_block_timestamp_valid_) {
        block_dur = packet::stream_timestamp_diff(curr_block_pkt->stream_timestamp(),
                                                  prev_block_timestamp_);
    }

    if (block_dur < 0) {
        roc_log(LogTrace,
                "fec block reader: negative block duration: prev_ts=%lu curr_ts=%lu",
                (unsigned long)prev_block_timestamp_,
                (unsigned long)curr_block_pkt->stream_timestamp());
        prev_block_timestamp_valid_ = false;
    } else {
        block_max_duration_ = std::max(block_max_duration_, block_dur);
        prev_block_timestamp_ = curr_block_pkt->stream_timestamp();
        prev_block_timestamp_valid_ = true;
    }
}

} // namespace fec
} // namespace roc
