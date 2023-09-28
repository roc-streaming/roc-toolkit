/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_PACKET_WRITER_H_
#define ROC_PIPELINE_TEST_HELPERS_PACKET_WRITER_H_

#include <CppUTest/TestHarness.h>

#include "roc_packet/units.h"
#include "test_helpers/utils.h"

#include "roc_audio/iframe_encoder.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_core/scoped_ptr.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_factory.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {
namespace test {

class PacketWriter : public core::NonCopyable<> {
public:
    PacketWriter(core::IArena& arena,
                 packet::IWriter& writer,
                 packet::IComposer& composer,
                 rtp::FormatMap& format_map,
                 packet::PacketFactory& packet_factory,
                 core::BufferFactory<uint8_t>& buffer_factory,
                 rtp::PayloadType pt,
                 const address::SocketAddr& src_addr,
                 const address::SocketAddr& dst_addr)
        : writer_(writer)
        , composer_(composer)
        , payload_encoder_(new_encoder_(arena, format_map, pt), arena)
        , packet_factory_(packet_factory)
        , buffer_factory_(buffer_factory)
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
                       const audio::SampleSpec& sample_spec) {
        CHECK(num_packets > 0);

        for (size_t np = 0; np < num_packets; np++) {
            writer_.write(new_packet_(samples_per_packet, sample_spec));
        }
    }

    void shift_to(size_t num_packets, size_t samples_per_packet) {
        seqnum_ = packet::seqnum_t(num_packets);
        timestamp_ = packet::stream_timestamp_t(num_packets * samples_per_packet);
        offset_ = uint8_t(num_packets * samples_per_packet);
    }

    uint8_t offset() const {
        return offset_;
    }

    void set_offset(size_t offset) {
        offset_ = uint8_t(offset);
    }

    packet::stream_source_t source() const {
        return source_;
    }

    void set_source(packet::stream_source_t source) {
        source_ = source;
    }

    packet::seqnum_t seqnum() const {
        return seqnum_;
    }

    void set_seqnum(packet::seqnum_t seqnum) {
        seqnum_ = seqnum;
    }

    packet::stream_timestamp_t timestamp() const {
        return timestamp_;
    }

    void set_timestamp(packet::stream_timestamp_t timestamp) {
        timestamp_ = timestamp;
    }

    void set_corrupt(bool corrupt) {
        corrupt_ = corrupt;
    }

private:
    enum { MaxSamples = 4096 };

    static audio::IFrameEncoder*
    new_encoder_(core::IArena& arena, rtp::FormatMap& format_map, rtp::PayloadType pt) {
        const rtp::Format* fmt = format_map.find_by_pt(pt);
        CHECK(fmt);

        return fmt->new_encoder(arena, fmt->pcm_format, fmt->sample_spec);
    }

    packet::PacketPtr new_packet_(size_t samples_per_packet,
                                  const audio::SampleSpec& sample_spec) {
        packet::PacketPtr pp = packet_factory_.new_packet();
        CHECK(pp);

        pp->add_flags(packet::Packet::FlagUDP);

        pp->udp()->src_addr = src_addr_;
        pp->udp()->dst_addr = dst_addr_;

        pp->set_data(new_buffer_(samples_per_packet, sample_spec));

        if (corrupt_) {
            pp->data().data()[0] = 0;
        }

        return pp;
    }

    core::Slice<uint8_t> new_buffer_(size_t samples_per_packet,
                                     const audio::SampleSpec& sample_spec) {
        CHECK(samples_per_packet * sample_spec.num_channels() < MaxSamples);

        packet::PacketPtr pp = packet_factory_.new_packet();
        CHECK(pp);

        core::Slice<uint8_t> bp = buffer_factory_.new_buffer();
        CHECK(bp);

        CHECK(composer_.prepare(
            *pp, bp, payload_encoder_->encoded_byte_count(samples_per_packet)));

        pp->set_data(bp);

        pp->rtp()->source = source_;
        pp->rtp()->seqnum = seqnum_;
        pp->rtp()->stream_timestamp = timestamp_;
        pp->rtp()->payload_type = pt_;

        seqnum_++;
        timestamp_ += samples_per_packet;

        audio::sample_t samples[MaxSamples];
        for (size_t ns = 0; ns < samples_per_packet; ns++) {
            for (size_t nc = 0; nc < sample_spec.num_channels(); nc++) {
                samples[ns * sample_spec.num_channels() + nc] = nth_sample(offset_);
            }
            offset_++;
        }

        payload_encoder_->begin(pp->rtp()->payload.data(), pp->rtp()->payload.size());

        UNSIGNED_LONGS_EQUAL(samples_per_packet,
                             payload_encoder_->write(samples, samples_per_packet));

        payload_encoder_->end();

        CHECK(composer_.compose(*pp));

        return pp->data();
    }

    packet::IWriter& writer_;

    packet::IComposer& composer_;
    core::ScopedPtr<audio::IFrameEncoder> payload_encoder_;

    packet::PacketFactory& packet_factory_;
    core::BufferFactory<uint8_t>& buffer_factory_;

    address::SocketAddr src_addr_;
    address::SocketAddr dst_addr_;

    packet::stream_source_t source_;
    packet::seqnum_t seqnum_;
    packet::stream_timestamp_t timestamp_;

    rtp::PayloadType pt_;

    uint8_t offset_;

    bool corrupt_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_PACKET_WRITER_H_
