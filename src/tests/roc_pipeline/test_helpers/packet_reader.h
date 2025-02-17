/*
 * Copyright (c) 2015 Roc Streaming authors
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
#include "roc_core/noncopyable.h"
#include "roc_core/scoped_ptr.h"
#include "roc_packet/iparser.h"
#include "roc_packet/ireader.h"
#include "roc_packet/packet_factory.h"
#include "roc_rtp/encoding_map.h"
#include "roc_rtp/parser.h"
#include "roc_status/status_code.h"

namespace roc {
namespace pipeline {
namespace test {

// Read, parse, and validates packets
class PacketReader : public core::NonCopyable<> {
public:
    PacketReader(core::IArena& arena,
                 packet::IReader& reader,
                 rtp::EncodingMap& encoding_map,
                 packet::PacketFactory& packet_factory,
                 const address::SocketAddr& dst_addr,
                 rtp::PayloadType pt)
        : reader_(reader)
        , packet_factory_(packet_factory)
        , dst_addr_(dst_addr)
        , source_(0)
        , seqnum_(0)
        , timestamp_(0)
        , pt_(pt)
        , offset_(0)
        , abs_offset_(0)
        , first_(true) {
        construct_(arena, encoding_map, pt);
    }

    void read_packet(size_t samples_per_packet,
                     const audio::SampleSpec& sample_spec,
                     core::nanoseconds_t base_capture_ts = -1) {
        packet::PacketPtr pp = read_packet_();

        audio::sample_t samples[MaxSamples] = {};
        parse_packet_(pp->buffer(), samples_per_packet, samples);
        check_capture_timestamp_(*pp, sample_spec, base_capture_ts);

        for (size_t ns = 0; ns < samples_per_packet; ns++) {
            for (size_t nc = 0; nc < sample_spec.num_channels(); nc++) {
                DOUBLES_EQUAL((double)nth_sample(offset_),
                              (double)samples[ns * sample_spec.num_channels() + nc],
                              SampleEpsilon);
            }
            offset_++;
        }
        abs_offset_ += samples_per_packet;
    }

    void read_nonzero_packet(size_t samples_per_packet,
                             const audio::SampleSpec& sample_spec,
                             core::nanoseconds_t base_capture_ts = -1) {
        packet::PacketPtr pp = read_packet_();

        audio::sample_t samples[MaxSamples] = {};
        parse_packet_(pp->buffer(), samples_per_packet, samples);
        check_capture_timestamp_(*pp, sample_spec, base_capture_ts);

        size_t non_zero = 0;
        for (size_t ns = 0; ns < samples_per_packet; ns++) {
            if (samples[ns] != 0) {
                non_zero++;
            }
        }
        CHECK(non_zero > 0);
        abs_offset_ += samples_per_packet;
    }

    void read_zero_packet(size_t samples_per_packet,
                          const audio::SampleSpec& sample_spec,
                          core::nanoseconds_t base_capture_ts = -1) {
        packet::PacketPtr pp = read_packet_();

        audio::sample_t samples[MaxSamples] = {};
        parse_packet_(pp->buffer(), samples_per_packet, samples);
        check_capture_timestamp_(*pp, sample_spec, base_capture_ts);

        for (size_t ns = 0; ns < samples_per_packet; ns++) {
            DOUBLES_EQUAL(0.0, (double)samples[ns], SampleEpsilon);
        }
        abs_offset_ += samples_per_packet;
    }

    void read_eof() {
        packet::PacketPtr pp;
        LONGS_EQUAL(status::StatusDrain, reader_.read(pp, packet::ModeFetch));
        CHECK(!pp);
    }

private:
    enum { MaxSamples = 4096 };

    void
    construct_(core::IArena& arena, rtp::EncodingMap& encoding_map, rtp::PayloadType pt) {
        // payload decoder
        const rtp::Encoding* enc = encoding_map.find_by_pt(pt);
        CHECK(enc);
        payload_decoder_.reset(enc->new_decoder(enc->sample_spec, arena));
        CHECK(payload_decoder_);

        // rtp parser
        parser_.reset(new (arena) rtp::Parser(NULL, encoding_map, arena));
    }

    packet::PacketPtr read_packet_() {
        packet::PacketPtr pp;
        LONGS_EQUAL(status::StatusOK, reader_.read(pp, packet::ModeFetch));
        CHECK(pp);

        CHECK(pp->flags() & packet::Packet::FlagUDP);
        CHECK(pp->flags() & packet::Packet::FlagComposed);

        CHECK(pp->udp()->dst_addr == dst_addr_);

        return pp;
    }

    void parse_packet_(const core::Slice<uint8_t> bp,
                       size_t samples_per_packet,
                       audio::sample_t* samples) {
        packet::PacketPtr pp = packet_factory_.new_packet();
        CHECK(pp);

        CHECK(parser_->parse(*pp, bp));
        CHECK(pp->flags() & packet::Packet::FlagRTP);

        if (first_) {
            source_ = pp->rtp()->source_id;
            seqnum_ = pp->rtp()->seqnum;
            timestamp_ = pp->rtp()->stream_timestamp;
            first_ = false;
        } else {
            UNSIGNED_LONGS_EQUAL(source_, pp->rtp()->source_id);
            UNSIGNED_LONGS_EQUAL(seqnum_, pp->rtp()->seqnum);
            UNSIGNED_LONGS_EQUAL(timestamp_, pp->rtp()->stream_timestamp);
        }

        UNSIGNED_LONGS_EQUAL(pt_, pp->rtp()->payload_type);

        seqnum_++;
        timestamp_ += samples_per_packet;

        payload_decoder_->begin_frame(pp->rtp()->stream_timestamp,
                                      pp->rtp()->payload.data(),
                                      pp->rtp()->payload.size());

        UNSIGNED_LONGS_EQUAL(samples_per_packet,
                             payload_decoder_->read_samples(samples, samples_per_packet));

        payload_decoder_->end_frame();
    }

    void check_capture_timestamp_(const packet::Packet& pkt,
                                  const audio::SampleSpec& sample_spec,
                                  core::nanoseconds_t base_ts) {
        CHECK(pkt.rtp());

        if (base_ts < 0) {
            LONGS_EQUAL(0, pkt.rtp()->capture_timestamp);
        } else {
            const core::nanoseconds_t capture_ts =
                base_ts + sample_spec.samples_per_chan_2_ns(abs_offset_);

            expect_capture_timestamp(capture_ts, pkt.rtp()->capture_timestamp,
                                     sample_spec, TimestampEpsilonSmpls);
        }
    }

    packet::IReader& reader_;

    core::ScopedPtr<packet::IParser> parser_;
    core::ScopedPtr<audio::IFrameDecoder> payload_decoder_;

    packet::PacketFactory& packet_factory_;

    address::SocketAddr dst_addr_;

    packet::stream_source_t source_;
    packet::seqnum_t seqnum_;
    packet::stream_timestamp_t timestamp_;

    rtp::PayloadType pt_;

    uint8_t offset_;
    size_t abs_offset_;
    bool first_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_PACKET_READER_H_
