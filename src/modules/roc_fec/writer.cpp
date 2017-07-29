/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
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

Writer::Writer(const Config& config,
               size_t payload_size,
               IEncoder& encoder,
               packet::IWriter& writer,
               packet::IComposer& source_composer,
               packet::IComposer& repair_composer,
               packet::PacketPool& packet_pool,
               core::BufferPool<uint8_t>& buffer_pool,
               core::IAllocator& allocator)
    : n_source_packets_(config.n_source_packets)
    , n_repair_packets_(config.n_repair_packets)
    , payload_size_(payload_size)
    , encoder_(encoder)
    , writer_(writer)
    , source_composer_(source_composer)
    , repair_composer_(repair_composer)
    , packet_pool_(packet_pool)
    , buffer_pool_(buffer_pool)
    , repair_packets_(allocator, config.n_repair_packets)
    , source_(0)
    , first_packet_(true)
    , cur_block_source_sn_(0)
    , cur_block_repair_sn_((packet::seqnum_t)core::random(packet::seqnum_t(-1)))
    , cur_packet_(0) {
    repair_packets_.resize(n_repair_packets_);
}

void Writer::write(const packet::PacketPtr& pp) {
    roc_panic_if_not(pp);

    if (!pp->rtp()) {
        roc_panic("fec writer: unexpected non-rtp packet");
    }

    if (!pp->fec()) {
        roc_panic("fec writer: unexpected non-fec packet");
    }

    if (first_packet_) {
        first_packet_ = false;
        do {
            source_ = (packet::source_t)core::random(packet::source_t(-1));
        } while (source_ == pp->rtp()->source);
    }

    if (cur_packet_ == 0) {
        cur_block_source_sn_ = pp->rtp()->seqnum;
        pp->rtp()->marker = true;
    }

    if (!source_composer_.compose(*pp)) {
        roc_panic("fec writer: can't compose packet");
    }

    pp->add_flags(packet::Packet::FlagComposed);

    writer_.write(pp);

    encoder_.set(cur_packet_, pp->fec()->payload);
    cur_packet_++;

    if (cur_packet_ == n_source_packets_) {
        for (packet::seqnum_t i = 0; i < n_repair_packets_; i++) {
            packet::PacketPtr rp = make_repair_packet_(i);
            if (!rp) {
                roc_log(LogDebug, "fec writer: can't create repair packet");
                continue;
            }
            repair_packets_[i] = rp;
            encoder_.set(cur_packet_ + i, rp->fec()->payload);
        }

        encoder_.commit();

        for (packet::seqnum_t i = 0; i < n_repair_packets_; i++) {
            packet::PacketPtr rp = repair_packets_[i];
            if (rp) {
                writer_.write(repair_packets_[i]);
                repair_packets_[i] = NULL;
            }
        }

        encoder_.reset();

        cur_block_repair_sn_ += n_repair_packets_;
        cur_packet_ = 0;
    }
}

packet::PacketPtr Writer::make_repair_packet_(packet::seqnum_t n) {
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

    if (!repair_composer_.prepare(*packet, data, payload_size_)) {
        roc_log(LogError, "fec writer: can't prepare packet");
        return NULL;
    }

    if (!packet->rtp()) {
        roc_panic("fec writer: unexpected non-rtp composer");
    }

    if (!packet->fec()) {
        roc_panic("fec writer: unexpected non-fec composer");
    }

    packet->set_data(data);

    packet::RTP& rtp = *packet->rtp();

    rtp.source = source_;
    rtp.seqnum = cur_block_repair_sn_ + n;
    rtp.marker = (n == 0);
    rtp.payload_type = 123;

    packet::FEC& fec = *packet->fec();

    fec.source_blknum = cur_block_source_sn_;
    fec.repair_blknum = cur_block_repair_sn_;

    return packet;
}

} // namespace fec
} // namespace roc
