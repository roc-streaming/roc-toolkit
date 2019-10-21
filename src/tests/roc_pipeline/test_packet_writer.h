/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_PACKET_WRITER_H_
#define ROC_PIPELINE_TEST_PACKET_WRITER_H_

#include <CppUTest/TestHarness.h>

#include "roc_audio/iframe_encoder.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/unique_ptr.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"
#include "roc_rtp/format_map.h"

#include "test_helpers.h"

namespace roc {
namespace pipeline {

class PacketWriter : public core::NonCopyable<> {
public:
    PacketWriter(core::IAllocator& allocator,
                 packet::IWriter& writer,
                 packet::IComposer& composer,
                 rtp::FormatMap& format_map,
                 packet::PacketPool& packet_pool,
                 core::BufferPool<uint8_t>& buffer_pool,
                 rtp::PayloadType pt,
                 const address::SocketAddr& src_addr,
                 const address::SocketAddr& dst_addr)
        : writer_(writer)
        , composer_(composer)
        , payload_encoder_(format_map.format(pt)->new_encoder(allocator), allocator)
        , packet_pool_(packet_pool)
        , buffer_pool_(buffer_pool)
        , src_addr_(src_addr)
        , dst_addr_(dst_addr)
        , source_(0)
        , seqnum_(0)
        , timestamp_(0)
        , pt_(pt)
        , offset_(0)
        , corrupt_(false) {
    }

    void write_packets(size_t num_packets,
                       size_t samples_per_packet,
                       packet::channel_mask_t channels) {
        CHECK(num_packets > 0);

        for (size_t np = 0; np < num_packets; np++) {
            writer_.write(new_packet_(samples_per_packet, channels));
        }
    }

    void shift_to(size_t num_packets,
                  size_t samples_per_packet,
                  packet::channel_mask_t channels) {
        seqnum_ = packet::seqnum_t(num_packets);
        timestamp_ = packet::timestamp_t(num_packets * samples_per_packet);
        offset_ =
            uint8_t(num_packets * samples_per_packet * packet::num_channels(channels));
    }

    uint8_t offset() const {
        return offset_;
    }

    void set_offset(size_t offset) {
        offset_ = uint8_t(offset);
    }

    void set_source(packet::source_t source) {
        source_ = source;
    }

    packet::seqnum_t seqnum() const {
        return seqnum_;
    }

    void set_seqnum(packet::seqnum_t seqnum) {
        seqnum_ = seqnum;
    }

    void set_timestamp(packet::timestamp_t timestamp) {
        timestamp_ = timestamp;
    }

    void set_corrupt(bool corrupt) {
        corrupt_ = corrupt;
    }

private:
    enum { MaxSamples = 4096 };

    packet::PacketPtr new_packet_(size_t samples_per_packet,
                                  packet::channel_mask_t channels) {
        packet::PacketPtr pp = new (packet_pool_) packet::Packet(packet_pool_);
        CHECK(pp);

        pp->add_flags(packet::Packet::FlagUDP);

        pp->udp()->src_addr = src_addr_;
        pp->udp()->dst_addr = dst_addr_;

        pp->set_data(new_buffer_(samples_per_packet, channels));

        if (corrupt_) {
            pp->data().data()[0] = 0;
        }

        return pp;
    }

    core::Slice<uint8_t> new_buffer_(size_t samples_per_packet,
                                     packet::channel_mask_t channels) {
        CHECK(samples_per_packet * packet::num_channels(channels) < MaxSamples);

        packet::PacketPtr pp = new (packet_pool_) packet::Packet(packet_pool_);
        CHECK(pp);

        core::Slice<uint8_t> bp = new (buffer_pool_) core::Buffer<uint8_t>(buffer_pool_);
        CHECK(bp);

        CHECK(composer_.prepare(*pp, bp,
                                payload_encoder_->encoded_size(samples_per_packet)));

        pp->set_data(bp);

        pp->rtp()->source = source_;
        pp->rtp()->seqnum = seqnum_;
        pp->rtp()->timestamp = timestamp_;
        pp->rtp()->payload_type = pt_;

        seqnum_++;
        timestamp_ += samples_per_packet;

        audio::sample_t samples[MaxSamples];
        for (size_t n = 0; n < samples_per_packet * packet::num_channels(channels); n++) {
            samples[n] = nth_sample(offset_++);
        }

        payload_encoder_->begin(pp->rtp()->payload.data(), pp->rtp()->payload.size());

        UNSIGNED_LONGS_EQUAL(
            samples_per_packet,
            payload_encoder_->write(samples, samples_per_packet, channels));

        payload_encoder_->end();

        CHECK(composer_.compose(*pp));

        return pp->data();
    }

    packet::IWriter& writer_;

    packet::IComposer& composer_;
    core::UniquePtr<audio::IFrameEncoder> payload_encoder_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& buffer_pool_;

    address::SocketAddr src_addr_;
    address::SocketAddr dst_addr_;

    packet::source_t source_;
    packet::seqnum_t seqnum_;
    packet::timestamp_t timestamp_;

    rtp::PayloadType pt_;

    uint8_t offset_;

    bool corrupt_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_PACKET_WRITER_H_
