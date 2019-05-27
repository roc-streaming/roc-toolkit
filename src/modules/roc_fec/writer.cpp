/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_fec/writer.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/random.h"

namespace roc {
namespace fec {

Writer::Writer(const WriterConfig& config,
               size_t payload_size,
               IEncoder& encoder,
               packet::IWriter& writer,
               packet::IComposer& source_composer,
               packet::IComposer& repair_composer,
               packet::PacketPool& packet_pool,
               core::BufferPool<uint8_t>& buffer_pool,
               core::IAllocator& allocator)
    : cur_sblen_(0)
    , next_sblen_(0)
    , cur_rblen_(0)
    , next_rblen_(0)
    , cur_payload_size_(0)
    , next_payload_size_(0)
    , encoder_(encoder)
    , writer_(writer)
    , source_composer_(source_composer)
    , repair_composer_(repair_composer)
    , packet_pool_(packet_pool)
    , buffer_pool_(buffer_pool)
    , repair_block_(allocator)
    , source_(0)
    , first_packet_(true)
    , cur_sbn_((packet::blknum_t)core::random(packet::blknum_t(-1)))
    , cur_block_repair_sn_((packet::seqnum_t)core::random(packet::seqnum_t(-1)))
    , cur_packet_(0)
    , valid_(false)
    , alive_(true) {
    if (!resize(config.n_source_packets, config.n_repair_packets, payload_size)) {
        return;
    }
    valid_ = true;
}

bool Writer::valid() const {
    return valid_;
}

bool Writer::alive() const {
    return alive_;
}

bool Writer::resize(size_t sblen, size_t rblen, size_t payload_size) {
    if (next_sblen_ == sblen && next_rblen_ == rblen
        && next_payload_size_ == payload_size) {
        return true;
    }

    if (payload_size == 0) {
        roc_log(LogError, "fec writer: resize: payload size can't be zero");
        return false;
    }

    if (sblen == 0) {
        roc_log(LogError, "fec writer: resize: sblen can't be zero");
        return false;
    }

    const size_t new_blen = sblen + rblen;

    if (new_blen > encoder_.max_block_length()) {
        roc_log(LogDebug,
                "fec writer: can't update block length, maximum value exceeded:"
                " cur_sbl=%lu cur_rbl=%lu new_sbl=%lu new_rbl=%lu max_blen=%lu",
                (unsigned long)cur_sblen_, (unsigned long)cur_rblen_,
                (unsigned long)sblen, (unsigned long)rblen,
                (unsigned long)encoder_.max_block_length());
        return false;
    }

    roc_log(LogDebug,
            "fec writer: update block size:"
            " cur_sbl=%lu cur_rbl=%lu new_sbl=%lu new_rbl=%lu payload_size=%lu",
            (unsigned long)cur_sblen_, (unsigned long)cur_rblen_, (unsigned long)sblen,
            (unsigned long)rblen, (unsigned long)payload_size);

    next_sblen_ = sblen;
    next_rblen_ = rblen;
    next_payload_size_ = payload_size;

    if (cur_packet_ == 0) {
        cur_sblen_ = sblen;
        cur_rblen_ = rblen;
        cur_payload_size_ = payload_size;
    }

    return true;
}

void Writer::write(const packet::PacketPtr& pp) {
    roc_panic_if_not(valid());
    roc_panic_if_not(pp);

    if (!alive_) {
        return;
    }

    if (!pp->rtp()) {
        roc_panic("fec writer: unexpected non-rtp packet");
    }

    if (!pp->fec()) {
        roc_panic("fec writer: unexpected non-fec packet");
    }

    if (first_packet_) {
        first_packet_ = false;
        generate_source_id_(pp);
    }

    if (cur_packet_ == 0) {
        if (!begin_block_()) {
            return;
        }
    }

    write_source_packet_(pp);

    cur_packet_++;

    if (cur_packet_ == cur_sblen_) {
        end_block_();
        next_block_();
    }
}

void Writer::generate_source_id_(const packet::PacketPtr& pp) {
    do {
        source_ = (packet::source_t)core::random(packet::source_t(-1));
    } while (source_ == pp->rtp()->source);
}

bool Writer::begin_block_() {
    if (!resize_repair_block_(cur_rblen_)) {
        return false;
    }

    if (!encoder_.begin(cur_sblen_, cur_rblen_, cur_payload_size_)) {
        roc_log(LogError,
                "fec writer: can't begin encoder block, shutting down:"
                " sblen=%lu rblen=%lu",
                (unsigned long)cur_sblen_, (unsigned long)cur_rblen_);
        return (alive_ = false);
    }

    return true;
}

void Writer::end_block_() {
    make_repair_packets_();
    encode_repair_packets_();
    compose_repair_packets_();
    write_repair_packets_();

    encoder_.end();
}

void Writer::next_block_() {
    cur_block_repair_sn_ += cur_rblen_;
    cur_sbn_++;

    cur_packet_ = 0;
    cur_sblen_ = next_sblen_;
    cur_rblen_ = next_rblen_;
    cur_payload_size_ = next_payload_size_;

    roc_log(LogTrace,
            "fec writer: next block: sbn=%lu sblen=%lu rblen=%lu payload_size=%lu",
            (unsigned long)cur_sbn_, (unsigned long)cur_sblen_, (unsigned long)cur_rblen_,
            (unsigned long)cur_payload_size_);
}

bool Writer::resize_repair_block_(size_t rblen) {
    if (repair_block_.size() == rblen) {
        return true;
    }

    if (!repair_block_.resize(rblen)) {
        roc_log(LogDebug,
                "fec writer: can't allocate repair block memory, shutting down:"
                " cur_rbl=%lu new_rbl=%lu",
                (unsigned long)repair_block_.size(), (unsigned long)rblen);
        return (alive_ = false);
    }

    return true;
}

void Writer::write_source_packet_(const packet::PacketPtr& pp) {
    encoder_.set(cur_packet_, pp->fec()->payload);

    pp->add_flags(packet::Packet::FlagComposed);
    fill_packet_fec_fields_(pp, (packet::seqnum_t)cur_packet_);

    if (!source_composer_.compose(*pp)) {
        roc_panic("fec writer: can't compose source packet");
    }

    writer_.write(pp);
}

void Writer::make_repair_packets_() {
    for (packet::seqnum_t i = 0; i < cur_rblen_; i++) {
        packet::PacketPtr rp = make_repair_packet_(i);
        if (!rp) {
            continue;
        }
        repair_block_[i] = rp;
    }
}

packet::PacketPtr Writer::make_repair_packet_(packet::seqnum_t pack_n) {
    packet::PacketPtr packet = new (packet_pool_) packet::Packet(packet_pool_);
    if (!packet) {
        roc_log(LogError, "fec writer: can't allocate packet");
        return NULL;
    }

    core::Slice<uint8_t> data = new (buffer_pool_) core::Buffer<uint8_t>(buffer_pool_);
    if (!data) {
        roc_log(LogError, "fec writer: can't allocate buffer");
        return NULL;
    }

    if (!repair_composer_.align(data, 0, encoder_.alignment())) {
        roc_log(LogError, "fec writer: can't align packet buffer");
        return NULL;
    }

    if (!repair_composer_.prepare(*packet, data, cur_payload_size_)) {
        roc_log(LogError, "fec writer: can't prepare packet");
        return NULL;
    }

    if (!packet->fec()) {
        roc_log(LogError, "fec writer: unexpected non-fec packet");
        return NULL;
    }

    packet->set_data(data);

    fill_packet_fec_fields_(packet, (packet::seqnum_t)cur_sblen_ + pack_n);
    return packet;
}

void Writer::encode_repair_packets_() {
    for (packet::seqnum_t i = 0; i < cur_rblen_; i++) {
        packet::PacketPtr rp = repair_block_[i];
        if (rp) {
            encoder_.set(cur_sblen_ + i, rp->fec()->payload);
        }
    }
    encoder_.fill();
}

void Writer::compose_repair_packets_() {
    for (size_t i = 0; i < cur_rblen_; i++) {
        packet::PacketPtr rp = repair_block_[i];
        if (!rp) {
            continue;
        }

        rp->add_flags(packet::Packet::FlagComposed);

        if (!repair_composer_.compose(*rp)) {
            roc_panic("fec writer: can't compose repair packet");
        }
    }
}

void Writer::write_repair_packets_() {
    for (packet::seqnum_t i = 0; i < cur_rblen_; i++) {
        packet::PacketPtr rp = repair_block_[i];
        if (rp) {
            writer_.write(repair_block_[i]);
            repair_block_[i] = NULL;
        }
    }
}

void Writer::fill_packet_fec_fields_(const packet::PacketPtr& packet,
                                     packet::seqnum_t pack_n) {
    packet::FEC& fec = *packet->fec();

    fec.encoding_symbol_id = pack_n;
    fec.source_block_number = cur_sbn_;
    fec.source_block_length = cur_sblen_;
    fec.block_length = cur_sblen_ + cur_rblen_;
}

} // namespace fec
} // namespace roc
