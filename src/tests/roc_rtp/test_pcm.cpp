/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_packet/packet_pool.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/pcm_decoder.h"
#include "roc_rtp/pcm_encoder.h"

namespace roc {
namespace rtp {

namespace {

enum { MaxBufsz = 100, MaxSamples = 100 };

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, MaxBufsz, 1);
packet::PacketPool packet_pool(allocator, 1);

} // namespace

TEST_GROUP(pcm) {
    audio::sample_t output[MaxSamples];

    template <class Sample, size_t NumCh>
    packet::PacketPtr new_packet(size_t num_samples) {
        packet::PacketPtr pp = new (packet_pool) packet::Packet(packet_pool);
        CHECK(pp);

        core::Slice<uint8_t> bp = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
        CHECK(bp);

        PCMEncoder<Sample, NumCh> encoder;
        const size_t payload_size = encoder.payload_size(num_samples);

        Composer composer(NULL);
        CHECK(composer.prepare(*pp, bp, payload_size));

        pp->set_data(bp);
        return pp;
    }

    template <class Sample, size_t NumCh>
    void encode(const packet::PacketPtr& pp, const audio::sample_t* samples,
                size_t offset, size_t num_samples, packet::channel_mask_t channels) {
        PCMEncoder<Sample, NumCh> encoder;

        UNSIGNED_LONGS_EQUAL(
            num_samples,
            encoder.write_samples(*pp, offset, samples, num_samples, channels));
    }

    template <class Sample, size_t NumCh>
    void decode(const packet::PacketPtr& pp, size_t offset, size_t num_samples,
                packet::channel_mask_t channels) {
        for (size_t i = 0; i < MaxSamples; i++) {
            output[i] = 0.0f;
        }

        PCMDecoder<Sample, NumCh> decoder;

        UNSIGNED_LONGS_EQUAL(
            num_samples,
            decoder.read_samples(*pp, offset, output, num_samples, channels));
    }

    void check(const audio::sample_t* samples, size_t num_samples,
               packet::channel_mask_t channels) {
        size_t n = 0;

        for (; n < num_samples * packet::num_channels(channels); n++) {
            DOUBLES_EQUAL(samples[n], output[n], 0.0001);
        }

        for (; n < MaxSamples; n++) {
            CHECK(output[n] == 0.0f);
        }
    }
};

TEST(pcm, payload_size) {
    enum { NumSamples = 77 };

    PCMEncoder<int16_t, 1> encoder1ch;
    UNSIGNED_LONGS_EQUAL(NumSamples * 1 * sizeof(int16_t),
                         encoder1ch.payload_size(NumSamples));

    PCMEncoder<int16_t, 2> encoder2ch;
    UNSIGNED_LONGS_EQUAL(NumSamples * 2 * sizeof(int16_t),
                         encoder2ch.payload_size(NumSamples));
}

TEST(pcm, 1ch) {
    enum { NumSamples = 5 };

    packet::PacketPtr pp = new_packet<int16_t, 1>(NumSamples);

    const audio::sample_t samples[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    encode<int16_t, 1>(pp, samples, 0, NumSamples, 0x1);
    decode<int16_t, 1>(pp, 0, NumSamples, 0x1);

    check(samples, NumSamples, 0x1);
}

TEST(pcm, 2ch) {
    enum { NumSamples = 5 };

    packet::PacketPtr pp = new_packet<int16_t, 2>(NumSamples);

    const audio::sample_t samples[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode<int16_t, 2>(pp, samples, 0, NumSamples, 0x3);
    decode<int16_t, 2>(pp, 0, NumSamples, 0x3);

    check(samples, NumSamples, 0x3);
}

TEST(pcm, encode_mask_subset) {
    enum { NumSamples = 5 };

    packet::PacketPtr pp = new_packet<int16_t, 2>(NumSamples);

    const audio::sample_t input[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    encode<int16_t, 2>(pp, input, 0, NumSamples, 0x2);
    decode<int16_t, 2>(pp, 0, NumSamples, 0x3);

    const audio::sample_t output[NumSamples * 2] = {
        0.0f, 0.1f, //
        0.0f, 0.2f, //
        0.0f, 0.3f, //
        0.0f, 0.4f, //
        0.0f, 0.5f, //
    };

    check(output, NumSamples, 0x3);
}

TEST(pcm, encode_mask_superset) {
    enum { NumSamples = 5 };

    packet::PacketPtr pp = new_packet<int16_t, 2>(NumSamples);

    const audio::sample_t input[NumSamples * 3] = {
        -0.1f, 0.1f, 0.8f, //
        -0.2f, 0.2f, 0.8f, //
        -0.3f, 0.3f, 0.8f, //
        -0.4f, 0.4f, 0.8f, //
        -0.5f, 0.5f, 0.8f, //
    };

    encode<int16_t, 2>(pp, input, 0, NumSamples, 0x7);
    decode<int16_t, 2>(pp, 0, NumSamples, 0x3);

    const audio::sample_t output[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    check(output, NumSamples, 0x3);
}

TEST(pcm, encode_mask_overlap) {
    enum { NumSamples = 5 };

    packet::PacketPtr pp = new_packet<int16_t, 2>(NumSamples);

    const audio::sample_t input[NumSamples * 3] = {
        -0.1f, 0.8f, //
        -0.2f, 0.8f, //
        -0.3f, 0.8f, //
        -0.4f, 0.8f, //
        -0.5f, 0.8f, //
    };

    encode<int16_t, 2>(pp, input, 0, NumSamples, 0x5);
    decode<int16_t, 2>(pp, 0, NumSamples, 0x3);

    const audio::sample_t output[NumSamples * 2] = {
        -0.1f, 0.0f, //
        -0.2f, 0.0f, //
        -0.3f, 0.0f, //
        -0.4f, 0.0f, //
        -0.5f, 0.0f, //
    };

    check(output, NumSamples, 0x3);
}

TEST(pcm, decode_mask_subset) {
    enum { NumSamples = 5 };

    packet::PacketPtr pp = new_packet<int16_t, 2>(NumSamples);

    const audio::sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode<int16_t, 2>(pp, input, 0, NumSamples, 0x3);
    decode<int16_t, 2>(pp, 0, NumSamples, 0x2);

    const audio::sample_t output[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    check(output, NumSamples, 0x2);
}

TEST(pcm, decode_mask_superset) {
    enum { NumSamples = 5 };

    packet::PacketPtr pp = new_packet<int16_t, 2>(NumSamples);

    const audio::sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode<int16_t, 2>(pp, input, 0, NumSamples, 0x3);
    decode<int16_t, 2>(pp, 0, NumSamples, 0x7);

    const audio::sample_t output[NumSamples * 3] = {
        -0.1f, 0.1f, 0.0f, //
        -0.2f, 0.2f, 0.0f, //
        -0.3f, 0.3f, 0.0f, //
        -0.4f, 0.4f, 0.0f, //
        -0.5f, 0.5f, 0.0f, //
    };

    check(output, NumSamples, 0x7);
}

TEST(pcm, decode_mask_overlap) {
    enum { NumSamples = 5 };

    packet::PacketPtr pp = new_packet<int16_t, 2>(NumSamples);

    const audio::sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode<int16_t, 2>(pp, input, 0, NumSamples, 0x3);
    decode<int16_t, 2>(pp, 0, NumSamples, 0x6);

    const audio::sample_t output[NumSamples * 2] = {
        0.1f, 0.0f, //
        0.2f, 0.0f, //
        0.3f, 0.0f, //
        0.4f, 0.0f, //
        0.5f, 0.0f, //
    };

    check(output, NumSamples, 0x6);
}

TEST(pcm, encode_incremental) {
    enum { NumSamples = 5, Off = 2 };

    packet::PacketPtr pp = new_packet<int16_t, 2>(NumSamples);

    const audio::sample_t input1[(NumSamples - Off) * 2] = {
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode<int16_t, 2>(pp, input1, Off, NumSamples - Off, 0x3);

    const audio::sample_t input2[Off] = {
        -0.1f, //
        -0.2f, //
    };

    encode<int16_t, 2>(pp, input2, 0, Off, 0x1);

    const audio::sample_t input3[Off] = {
        0.1f, //
        0.2f, //
    };

    encode<int16_t, 2>(pp, input3, 0, Off, 0x2);

    const audio::sample_t output[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    decode<int16_t, 2>(pp, 0, NumSamples, 0x3);

    check(output, NumSamples, 0x3);
}

TEST(pcm, decode_incremenal) {
    enum { NumSamples = 5, Off = 2 };

    packet::PacketPtr pp = new_packet<int16_t, 2>(NumSamples);

    const audio::sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode<int16_t, 2>(pp, input, 0, NumSamples, 0x3);

    decode<int16_t, 2>(pp, 0, Off, 0x3);

    const audio::sample_t output1[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
    };

    check(output1, Off, 0x3);

    decode<int16_t, 2>(pp, Off, NumSamples - Off, 0x1);

    const audio::sample_t output2[NumSamples] = {
        -0.3f, //
        -0.4f, //
        -0.5f, //
    };

    check(output2, NumSamples - Off, 0x1);

    decode<int16_t, 2>(pp, Off, NumSamples - Off, 0x2);

    const audio::sample_t output3[NumSamples] = {
        0.3f, //
        0.4f, //
        0.5f, //
    };

    check(output3, NumSamples - Off, 0x2);
}

TEST(pcm, encode_truncate) {
    enum { NumSamples = 5, Off = 2 };

    packet::PacketPtr pp = new_packet<int16_t, 2>(NumSamples);

    const audio::sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    PCMEncoder<int16_t, 2> encoder;

    UNSIGNED_LONGS_EQUAL(NumSamples - Off,
                         encoder.write_samples(*pp, Off, input, NumSamples, 0x3));

    UNSIGNED_LONGS_EQUAL(0, encoder.write_samples(*pp, NumSamples, input, 123, 0x3));

    const audio::sample_t output[NumSamples * 2] = {
        0.0f,  0.0f, //
        0.0f,  0.0f, //
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
    };

    decode<int16_t, 2>(pp, 0, NumSamples, 0x3);

    check(output, NumSamples, 0x3);
}

TEST(pcm, decode_truncate) {
    enum { NumSamples = 5, Off = 2 };

    packet::PacketPtr pp = new_packet<int16_t, 2>(NumSamples);

    const audio::sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode<int16_t, 2>(pp, input, 0, NumSamples, 0x3);

    PCMDecoder<int16_t, 2> decoder;

    UNSIGNED_LONGS_EQUAL(NumSamples - Off,
                         decoder.read_samples(*pp, Off, output, NumSamples, 0x3));

    UNSIGNED_LONGS_EQUAL(0, decoder.read_samples(*pp, NumSamples, output, 123, 0x3));

    const audio::sample_t output[NumSamples * 2] = {
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
        0.0f,  0.0f, //
        0.0f,  0.0f, //
    };

    check(output, NumSamples, 0x3);
}

} // namespace rtp
} // namespace roc
