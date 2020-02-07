/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_PACKET_READER_H_
#define ROC_PIPELINE_TEST_HELPERS_PACKET_READER_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_audio/iframe_decoder.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/scoped_ptr.h"
#include "roc_packet/iparser.h"
#include "roc_packet/ireader.h"
#include "roc_packet/packet_pool.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {
namespace test {

class PacketReader : public core::NonCopyable<> {
public:
    PacketReader(core::IAllocator& allocator,
                 packet::IReader& reader,
                 packet::IParser& parser,
                 rtp::FormatMap& format_map,
                 packet::PacketPool& packet_pool,
                 rtp::PayloadType pt,
                 const address::SocketAddr& dst_addr)
        : reader_(reader)
        , parser_(parser)
        , payload_decoder_(format_map.format(pt)->new_decoder(allocator), allocator)
        , packet_pool_(packet_pool)
        , dst_addr_(dst_addr)
        , source_(0)
        , seqnum_(0)
        , timestamp_(0)
        , pt_(pt)
        , offset_(0)
        , first_(true) {
    }

    void read_packet(size_t samples_per_packet, packet::channel_mask_t channels) {
        packet::PacketPtr pp = reader_.read();
        CHECK(pp);

        CHECK(pp->flags() & packet::Packet::FlagUDP);
        CHECK(pp->udp()->dst_addr == dst_addr_);

        CHECK(pp->flags() & packet::Packet::FlagComposed);
        check_buffer_(pp->data(), samples_per_packet, channels);
    }

private:
    enum { MaxSamples = 4096 };

    void check_buffer_(const core::Slice<uint8_t> bp,
                       size_t samples_per_packet,
                       packet::channel_mask_t channels) {
        packet::PacketPtr pp = new (packet_pool_) packet::Packet(packet_pool_);
        CHECK(pp);

        CHECK(parser_.parse(*pp, bp));
        CHECK(pp->flags() & packet::Packet::FlagRTP);

        if (first_) {
            source_ = pp->rtp()->source;
            seqnum_ = pp->rtp()->seqnum;
            timestamp_ = pp->rtp()->timestamp;
            first_ = false;
        } else {
            UNSIGNED_LONGS_EQUAL(source_, pp->rtp()->source);
            UNSIGNED_LONGS_EQUAL(seqnum_, pp->rtp()->seqnum);
            UNSIGNED_LONGS_EQUAL(timestamp_, pp->rtp()->timestamp);
        }

        UNSIGNED_LONGS_EQUAL(pt_, pp->rtp()->payload_type);
        UNSIGNED_LONGS_EQUAL(samples_per_packet, pp->rtp()->duration);

        seqnum_++;
        timestamp_ += samples_per_packet;

        payload_decoder_->begin(pp->rtp()->timestamp, pp->rtp()->payload.data(),
                                pp->rtp()->payload.size());

        audio::sample_t samples[MaxSamples] = {};
        UNSIGNED_LONGS_EQUAL(
            samples_per_packet,
            payload_decoder_->read(samples, samples_per_packet, channels));

        payload_decoder_->end();

        for (size_t n = 0; n < samples_per_packet * packet::num_channels(channels); n++) {
            DOUBLES_EQUAL((double)nth_sample(offset_), (double)samples[n], Epsilon);
            offset_++;
        }
    }

    packet::IReader& reader_;

    packet::IParser& parser_;
    core::ScopedPtr<audio::IFrameDecoder> payload_decoder_;

    packet::PacketPool& packet_pool_;

    address::SocketAddr dst_addr_;

    packet::source_t source_;
    packet::seqnum_t seqnum_;
    packet::timestamp_t timestamp_;

    rtp::PayloadType pt_;

    uint8_t offset_;
    bool first_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_PACKET_READER_H_
