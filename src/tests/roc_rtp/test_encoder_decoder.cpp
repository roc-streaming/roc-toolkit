/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/unique_ptr.h"
#include "roc_packet/packet_pool.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/pcm_decoder.h"
#include "roc_rtp/pcm_encoder.h"
#include "roc_rtp/pcm_funcs.h"

namespace roc {
namespace rtp {

namespace {

enum {
    Test_PCM_16bit_1ch,
    Test_PCM_16bit_2ch,

    Test_NumCodecs
};

packet::channel_mask_t Test_codec_channels[Test_NumCodecs] = {
    0x1,
    0x3
};

unsigned int Test_codec_pts[Test_NumCodecs] = {
    11,
    10
};

enum { MaxChans = 8, MaxBufSize = 1000 };

const double Epsilon = 0.00001;

core::HeapAllocator allocator;
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, true);
packet::PacketPool packet_pool(allocator, true);

Composer rtp_composer(NULL);
FormatMap format_map;

audio::sample_t nth_sample(uint8_t n) {
    return audio::sample_t(n) / audio::sample_t(1 << 8);
}

} // namespace

TEST_GROUP(encoder) {
    const Format& get_format(unsigned int pt) {
        const Format* fmt = format_map.format(pt);
        CHECK(fmt);
        return *fmt;
    }

    audio::IEncoder* new_encoder(size_t id) {
        switch (id) {
        case Test_PCM_16bit_1ch:
            return new (allocator)
                PCMEncoder(PCM_16bit_1ch, get_format(PayloadType_L16_Mono));

        case Test_PCM_16bit_2ch:
            return new (allocator)
                PCMEncoder(PCM_16bit_2ch, get_format(PayloadType_L16_Stereo));

        default:
            FAIL("bad codec id");
        }

        return NULL;
    }

    audio::IDecoder* new_decoder(size_t id) {
        switch (id) {
        case Test_PCM_16bit_1ch:
            return new (allocator)
                PCMDecoder(PCM_16bit_1ch, get_format(PayloadType_L16_Mono));

        case Test_PCM_16bit_2ch:
            return new (allocator)
                PCMDecoder(PCM_16bit_2ch, get_format(PayloadType_L16_Stereo));

        default:
            FAIL("bad codec id");
        }

        return NULL;
    }

    packet::PacketPtr new_packet(size_t payload_size) {
        packet::PacketPtr pp = new(packet_pool) packet::Packet(packet_pool);
        CHECK(pp);

        core::Slice<uint8_t> bp =
            new (byte_buffer_pool) core::Buffer<uint8_t>(byte_buffer_pool);
        CHECK(bp);

        CHECK(rtp_composer.prepare(*pp, bp, payload_size));

        pp->set_data(bp);

        return pp;
    }

    size_t fill_samples(audio::sample_t* samples,
                        size_t pos,
                        size_t n_samples,
                        packet::channel_mask_t ch_mask) {
        const size_t n_chans = packet::num_channels(ch_mask);

        for (size_t i = 0; i < n_samples; i++) {
            for (size_t j = 0; j < n_chans; j++) {
                *samples++ = nth_sample(uint8_t(pos++));
            }
        }

        return pos;
    }

    size_t check_samples(const audio::sample_t* samples,
                         size_t pos,
                         size_t n_samples,
                         packet::channel_mask_t ch_mask) {
        const size_t n_chans = packet::num_channels(ch_mask);

        for (size_t i = 0; i < n_samples; i++) {
            for (size_t j = 0; j < n_chans; j++) {
                audio::sample_t actual = *samples++;
                audio::sample_t expected = nth_sample(uint8_t(pos++));

                DOUBLES_EQUAL(expected, actual, Epsilon);
            }
        }

        return pos;
    }

    size_t check_zeros(const audio::sample_t* samples, size_t pos, size_t n_samples) {
        for (size_t i = 0; i < n_samples; i++) {
            audio::sample_t actual = *samples++;
            DOUBLES_EQUAL(0.0, actual, Epsilon);
            pos++;
        }

        return pos;
    }
};

TEST(encoder, one_packet) {
    enum { SamplesPerPacket = 177 };

    for (size_t n_codec = 0; n_codec < Test_NumCodecs; n_codec++) {
        core::UniquePtr<audio::IEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::UniquePtr<audio::IDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        packet::PacketPtr pp = new_packet(encoder->payload_size(SamplesPerPacket));

        encoder->begin(pp);

        audio::sample_t encoder_samples[SamplesPerPacket * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerPacket, Test_codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(SamplesPerPacket,
                             encoder->write(encoder_samples, SamplesPerPacket,
                                            Test_codec_channels[n_codec]));

        encoder->end();

        UNSIGNED_LONGS_EQUAL(Test_codec_pts[n_codec], pp->rtp()->payload_type);

        audio::sample_t decoder_samples[SamplesPerPacket * MaxChans];

        UNSIGNED_LONGS_EQUAL(SamplesPerPacket,
                             decoder->read_samples(*pp, 0, decoder_samples,
                                                   SamplesPerPacket,
                                                   Test_codec_channels[n_codec]));

        check_samples(decoder_samples, 0, SamplesPerPacket, Test_codec_channels[n_codec]);
    }
}

TEST(encoder, multiple_packets) {
    enum { NumPackets = 10, SamplesPerPacket = 177 };

    for (size_t n_codec = 0; n_codec < Test_NumCodecs; n_codec++) {
        core::UniquePtr<audio::IEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::UniquePtr<audio::IDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        packet::source_t src = 0;
        packet::seqnum_t sn = 0;
        packet::timestamp_t ts = 0;

        size_t encoder_pos = 0;
        size_t decoder_pos = 0;

        for (size_t n = 0; n < NumPackets; n++) {
            packet::PacketPtr pp = new_packet(encoder->payload_size(SamplesPerPacket));

            encoder->begin(pp);


            audio::sample_t encoder_samples[SamplesPerPacket * MaxChans] = {};
            encoder_pos = fill_samples(encoder_samples, encoder_pos, SamplesPerPacket,
                                       Test_codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(SamplesPerPacket,
                                 encoder->write(encoder_samples, SamplesPerPacket,
                                                Test_codec_channels[n_codec]));

            encoder->end();

            if (n == 0) {
                src = pp->rtp()->source;
                sn = pp->rtp()->seqnum;
                ts = pp->rtp()->timestamp;
            }

            UNSIGNED_LONGS_EQUAL(Test_codec_pts[n_codec], pp->rtp()->payload_type);
            UNSIGNED_LONGS_EQUAL(src, pp->rtp()->source);
            UNSIGNED_LONGS_EQUAL(sn, pp->rtp()->seqnum);
            UNSIGNED_LONGS_EQUAL(ts, pp->rtp()->timestamp);

            sn++;
            ts += SamplesPerPacket;


            audio::sample_t decoder_samples[SamplesPerPacket * MaxChans];

            UNSIGNED_LONGS_EQUAL(SamplesPerPacket,
                                 decoder->read_samples(*pp, 0, decoder_samples,
                                                       SamplesPerPacket,
                                                       Test_codec_channels[n_codec]));

            decoder_pos = check_samples(decoder_samples, decoder_pos, SamplesPerPacket,
                                        Test_codec_channels[n_codec]);

            UNSIGNED_LONGS_EQUAL(encoder_pos, decoder_pos);
        }
    }
}

TEST(encoder, write_incrementally) {
    enum { FirstPart = 33, SecondPart = 44, SamplesPerPacket = FirstPart + SecondPart };

    for (size_t n_codec = 0; n_codec < Test_NumCodecs; n_codec++) {
        core::UniquePtr<audio::IEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::UniquePtr<audio::IDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        packet::PacketPtr pp = new_packet(encoder->payload_size(SamplesPerPacket));

        encoder->begin(pp);

        audio::sample_t encoder_samples[SamplesPerPacket * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerPacket, Test_codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(
            FirstPart,
            encoder->write(encoder_samples, FirstPart, Test_codec_channels[n_codec]));

        UNSIGNED_LONGS_EQUAL(
            SecondPart,
            encoder->write(encoder_samples
                               + FirstPart
                                   * packet::num_channels(Test_codec_channels[n_codec]),
                           SecondPart, Test_codec_channels[n_codec]));

        encoder->end();

        audio::sample_t decoder_samples[SamplesPerPacket * MaxChans];

        UNSIGNED_LONGS_EQUAL(SamplesPerPacket,
                             decoder->read_samples(*pp, 0, decoder_samples,
                                                   SamplesPerPacket,
                                                   Test_codec_channels[n_codec]));

        check_samples(decoder_samples, 0, SamplesPerPacket, Test_codec_channels[n_codec]);
    }
}

TEST(encoder, write_too_much) {
    enum { SamplesPerPacket = 177 };

    for (size_t n_codec = 0; n_codec < Test_NumCodecs; n_codec++) {
        core::UniquePtr<audio::IEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::UniquePtr<audio::IDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        packet::PacketPtr pp = new_packet(encoder->payload_size(SamplesPerPacket));

        encoder->begin(pp);

        audio::sample_t encoder_samples[(SamplesPerPacket + 20) * MaxChans] = {};
        fill_samples(encoder_samples, 0, SamplesPerPacket + 20,
                     Test_codec_channels[n_codec]);

        UNSIGNED_LONGS_EQUAL(SamplesPerPacket,
                             encoder->write(encoder_samples, SamplesPerPacket + 20,
                                            Test_codec_channels[n_codec]));

        encoder->end();

        audio::sample_t decoder_samples[SamplesPerPacket * MaxChans];

        UNSIGNED_LONGS_EQUAL(SamplesPerPacket,
                             decoder->read_samples(*pp, 0, decoder_samples,
                                                   SamplesPerPacket,
                                                   Test_codec_channels[n_codec]));

        check_samples(decoder_samples, 0, SamplesPerPacket, Test_codec_channels[n_codec]);
    }
}

TEST(encoder, write_channel_mask) {
    enum {
        FirstPart = 33,
        SecondPart = 44,
        FirstPartChans = 0xff,
        SecondPartChans = 0x1,
        SamplesPerPacket = FirstPart + SecondPart
    };

    for (size_t n_codec = 0; n_codec < Test_NumCodecs; n_codec++) {
        core::UniquePtr<audio::IEncoder> encoder(new_encoder(n_codec), allocator);
        CHECK(encoder);

        core::UniquePtr<audio::IDecoder> decoder(new_decoder(n_codec), allocator);
        CHECK(decoder);

        packet::PacketPtr pp = new_packet(encoder->payload_size(SamplesPerPacket));

        encoder->begin(pp);

        size_t encoder_pos = 0;

        {
            audio::sample_t encoder_samples[FirstPart * MaxChans] = {};
            encoder_pos =
                fill_samples(encoder_samples, encoder_pos, FirstPart, FirstPartChans);

            UNSIGNED_LONGS_EQUAL(
                FirstPart, encoder->write(encoder_samples, FirstPart, FirstPartChans));
        }

        {
            audio::sample_t encoder_samples[SecondPart * MaxChans] = {};
            encoder_pos =
                fill_samples(encoder_samples, encoder_pos, SecondPart, SecondPartChans);

            UNSIGNED_LONGS_EQUAL(
                SecondPart, encoder->write(encoder_samples, SecondPart, SecondPartChans));
        }

        encoder->end();

        audio::sample_t decoder_samples[SamplesPerPacket * MaxChans];

        UNSIGNED_LONGS_EQUAL(SamplesPerPacket,
                             decoder->read_samples(*pp, 0, decoder_samples,
                                                   SamplesPerPacket,
                                                   Test_codec_channels[n_codec]));

        size_t actual_pos = 0;
        size_t expected_pos = 0;

        for (size_t i = 0; i < FirstPart; i++) {
            for (size_t j = 0; j < packet::num_channels(FirstPartChans); j++) {
                if (Test_codec_channels[n_codec] & (1 << j)) {
                    audio::sample_t actual = decoder_samples[actual_pos++];
                    audio::sample_t expected = nth_sample(uint8_t(expected_pos));

                    DOUBLES_EQUAL(expected, actual, Epsilon);
                }

                expected_pos++;
            }
        }

        for (size_t i = FirstPart; i < SamplesPerPacket; i++) {
            for (size_t j = 0; j < packet::num_channels(Test_codec_channels[n_codec]);
                 j++) {
                audio::sample_t actual = decoder_samples[actual_pos++];
                audio::sample_t expected = 0;

                if (SecondPartChans & (1 << j)) {
                    expected = nth_sample(uint8_t(expected_pos++));
                }

                DOUBLES_EQUAL(expected, actual, Epsilon);
            }
        }
    }
}

} // namespace rtp
} // namespace roc
