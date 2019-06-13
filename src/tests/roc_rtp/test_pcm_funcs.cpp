/*
 * Copyright (c) 2015 Roc authors
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

const double Epsilon = 0.0001;

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, MaxBufsz, true);
packet::PacketPool packet_pool(allocator, true);

} // namespace

TEST_GROUP(pcm_funcs) {
    const PCMFuncs* funcs;
    audio::sample_t output[MaxSamples];

    void use(const PCMFuncs& f) {
        funcs = &f;
    }

    packet::PacketPtr new_packet(size_t num_samples) {
        CHECK(funcs);

        packet::PacketPtr pp = new (packet_pool) packet::Packet(packet_pool);
        CHECK(pp);

        core::Slice<uint8_t> bp = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
        CHECK(bp);

        for (size_t n = 0; n < bp.size(); n++) {
            bp.data()[n] = 0;
        }

        const size_t payload_size = funcs->payload_size_from_samples(num_samples);

        Composer composer(NULL);
        CHECK(composer.prepare(*pp, bp, payload_size));

        pp->set_data(bp);
        return pp;
    }

    void encode(const packet::PacketPtr& pp, const audio::sample_t* samples,
                size_t offset, size_t num_samples, packet::channel_mask_t channels) {
        CHECK(funcs);

        UNSIGNED_LONGS_EQUAL(num_samples,
                             funcs->encode_samples(pp->rtp()->payload.data(),
                                                   pp->rtp()->payload.size(), offset,
                                                   samples, num_samples, channels));
    }

    void decode(const packet::PacketPtr& pp, size_t offset, size_t num_samples,
                packet::channel_mask_t channels) {
        CHECK(funcs);

        for (size_t i = 0; i < MaxSamples; i++) {
            output[i] = 0.0f;
        }

        UNSIGNED_LONGS_EQUAL(num_samples,
                             funcs->decode_samples(pp->rtp()->payload.data(),
                                                   pp->rtp()->payload.size(), offset,
                                                   output, num_samples, channels));
    }

    void check(const audio::sample_t* samples, size_t num_samples,
               packet::channel_mask_t channels) {
        size_t n = 0;

        for (; n < num_samples * packet::num_channels(channels); n++) {
            DOUBLES_EQUAL((double)samples[n], (double)output[n], Epsilon);
        }

        for (; n < MaxSamples; n++) {
            DOUBLES_EQUAL(0.0, (double)output[n], Epsilon);
        }
    }
};

TEST(pcm_funcs, payload_size_1ch) {
    enum { NumSamples = 77 };

    use(PCM_16bit_1ch);

    UNSIGNED_LONGS_EQUAL(NumSamples * 1 * sizeof(int16_t),
                         funcs->payload_size_from_samples(NumSamples));
}

TEST(pcm_funcs, payload_size_2ch) {
    enum { NumSamples = 77 };

    use(PCM_16bit_2ch);

    UNSIGNED_LONGS_EQUAL(NumSamples * 2 * sizeof(int16_t),
                         funcs->payload_size_from_samples(NumSamples));
}

TEST(pcm_funcs, encode_decode_1ch) {
    enum { NumSamples = 5 };

    use(PCM_16bit_1ch);

    packet::PacketPtr pp = new_packet(NumSamples);

    const audio::sample_t samples[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    encode(pp, samples, 0, NumSamples, 0x1);
    decode(pp, 0, NumSamples, 0x1);

    check(samples, NumSamples, 0x1);
}

TEST(pcm_funcs, encode_decode_2ch) {
    enum { NumSamples = 5 };

    use(PCM_16bit_2ch);

    packet::PacketPtr pp = new_packet(NumSamples);

    const audio::sample_t samples[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(pp, samples, 0, NumSamples, 0x3);
    decode(pp, 0, NumSamples, 0x3);

    check(samples, NumSamples, 0x3);
}

TEST(pcm_funcs, encode_mask_subset) {
    enum { NumSamples = 5 };

    use(PCM_16bit_2ch);

    packet::PacketPtr pp = new_packet(NumSamples);

    const audio::sample_t input[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    encode(pp, input, 0, NumSamples, 0x2);
    decode(pp, 0, NumSamples, 0x3);

    const audio::sample_t output[NumSamples * 2] = {
        0.0f, 0.1f, //
        0.0f, 0.2f, //
        0.0f, 0.3f, //
        0.0f, 0.4f, //
        0.0f, 0.5f, //
    };

    check(output, NumSamples, 0x3);
}

TEST(pcm_funcs, encode_mask_superset) {
    enum { NumSamples = 5 };

    use(PCM_16bit_2ch);

    packet::PacketPtr pp = new_packet(NumSamples);

    const audio::sample_t input[NumSamples * 3] = {
        -0.1f, 0.1f, 0.8f, //
        -0.2f, 0.2f, 0.8f, //
        -0.3f, 0.3f, 0.8f, //
        -0.4f, 0.4f, 0.8f, //
        -0.5f, 0.5f, 0.8f, //
    };

    encode(pp, input, 0, NumSamples, 0x7);
    decode(pp, 0, NumSamples, 0x3);

    const audio::sample_t output[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    check(output, NumSamples, 0x3);
}

TEST(pcm_funcs, encode_mask_overlap) {
    enum { NumSamples = 5 };

    use(PCM_16bit_2ch);

    packet::PacketPtr pp = new_packet(NumSamples);

    const audio::sample_t input[NumSamples * 3] = {
        -0.1f, 0.8f, //
        -0.2f, 0.8f, //
        -0.3f, 0.8f, //
        -0.4f, 0.8f, //
        -0.5f, 0.8f, //
    };

    encode(pp, input, 0, NumSamples, 0x5);
    decode(pp, 0, NumSamples, 0x3);

    const audio::sample_t output[NumSamples * 2] = {
        -0.1f, 0.0f, //
        -0.2f, 0.0f, //
        -0.3f, 0.0f, //
        -0.4f, 0.0f, //
        -0.5f, 0.0f, //
    };

    check(output, NumSamples, 0x3);
}

TEST(pcm_funcs, decode_mask_subset) {
    enum { NumSamples = 5 };

    use(PCM_16bit_2ch);

    packet::PacketPtr pp = new_packet(NumSamples);

    const audio::sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(pp, input, 0, NumSamples, 0x3);
    decode(pp, 0, NumSamples, 0x2);

    const audio::sample_t output[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    check(output, NumSamples, 0x2);
}

TEST(pcm_funcs, decode_mask_superset) {
    enum { NumSamples = 5 };

    use(PCM_16bit_2ch);

    packet::PacketPtr pp = new_packet(NumSamples);

    const audio::sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(pp, input, 0, NumSamples, 0x3);
    decode(pp, 0, NumSamples, 0x7);

    const audio::sample_t output[NumSamples * 3] = {
        -0.1f, 0.1f, 0.0f, //
        -0.2f, 0.2f, 0.0f, //
        -0.3f, 0.3f, 0.0f, //
        -0.4f, 0.4f, 0.0f, //
        -0.5f, 0.5f, 0.0f, //
    };

    check(output, NumSamples, 0x7);
}

TEST(pcm_funcs, decode_mask_overlap) {
    enum { NumSamples = 5 };

    use(PCM_16bit_2ch);

    packet::PacketPtr pp = new_packet(NumSamples);

    const audio::sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(pp, input, 0, NumSamples, 0x3);
    decode(pp, 0, NumSamples, 0x6);

    const audio::sample_t output[NumSamples * 2] = {
        0.1f, 0.0f, //
        0.2f, 0.0f, //
        0.3f, 0.0f, //
        0.4f, 0.0f, //
        0.5f, 0.0f, //
    };

    check(output, NumSamples, 0x6);
}

TEST(pcm_funcs, encode_incremental) {
    enum { NumSamples = 5, Off = 2 };

    use(PCM_16bit_2ch);

    packet::PacketPtr pp = new_packet(NumSamples);

    const audio::sample_t input1[(NumSamples - Off) * 2] = {
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(pp, input1, Off, NumSamples - Off, 0x3);

    const audio::sample_t input2[Off] = {
        -0.1f, //
        -0.2f, //
    };

    encode(pp, input2, 0, Off, 0x1);

    const audio::sample_t output[NumSamples * 2] = {
        -0.1f, 0.0f, //
        -0.2f, 0.0f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    decode(pp, 0, NumSamples, 0x3);

    check(output, NumSamples, 0x3);
}

TEST(pcm_funcs, decode_incremenal) {
    enum { NumSamples = 5, Off = 2 };

    use(PCM_16bit_2ch);

    packet::PacketPtr pp = new_packet(NumSamples);

    const audio::sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(pp, input, 0, NumSamples, 0x3);

    decode(pp, 0, Off, 0x3);

    const audio::sample_t output1[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
    };

    check(output1, Off, 0x3);

    decode(pp, Off, NumSamples - Off, 0x1);

    const audio::sample_t output2[NumSamples] = {
        -0.3f, //
        -0.4f, //
        -0.5f, //
    };

    check(output2, NumSamples - Off, 0x1);

    decode(pp, Off, NumSamples - Off, 0x2);

    const audio::sample_t output3[NumSamples] = {
        0.3f, //
        0.4f, //
        0.5f, //
    };

    check(output3, NumSamples - Off, 0x2);
}

TEST(pcm_funcs, encode_truncate) {
    enum { NumSamples = 5, Off = 2 };

    use(PCM_16bit_2ch);

    packet::PacketPtr pp = new_packet(NumSamples);

    const audio::sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    UNSIGNED_LONGS_EQUAL(NumSamples - Off,
                         funcs->encode_samples(pp->rtp()->payload.data(),
                                               pp->rtp()->payload.size(), Off, input,
                                               NumSamples, 0x3));

    UNSIGNED_LONGS_EQUAL(0,
                         funcs->encode_samples(pp->rtp()->payload.data(),
                                               pp->rtp()->payload.size(), 123, input,
                                               NumSamples, 0x3));

    const audio::sample_t output[NumSamples * 2] = {
        0.0f,  0.0f, //
        0.0f,  0.0f, //
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
    };

    decode(pp, 0, NumSamples, 0x3);

    check(output, NumSamples, 0x3);
}

TEST(pcm_funcs, decode_truncate) {
    enum { NumSamples = 5, Off = 2 };

    use(PCM_16bit_2ch);

    packet::PacketPtr pp = new_packet(NumSamples);

    const audio::sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(pp, input, 0, NumSamples, 0x3);

    UNSIGNED_LONGS_EQUAL(NumSamples - Off,
                         funcs->decode_samples(pp->rtp()->payload.data(),
                                               pp->rtp()->payload.size(), Off, output,
                                               NumSamples, 0x3));

    UNSIGNED_LONGS_EQUAL(0,
                         funcs->decode_samples(pp->rtp()->payload.data(),
                                               pp->rtp()->payload.size(), 123, output,
                                               NumSamples, 0x3));

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
