/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_encoder.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"

namespace roc {
namespace audio {

namespace {

enum { MaxBufsz = 100, MaxSamples = 100 };

const double Epsilon = 0.0001;

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, MaxBufsz, true);

} // namespace

TEST_GROUP(pcm_funcs) {
    const PcmFuncs* funcs;
    sample_t output[MaxSamples];

    void use(const PcmFuncs& f) {
        funcs = &f;
    }

    core::Slice<uint8_t> new_buffer(size_t num_samples) {
        CHECK(funcs);

        core::Slice<uint8_t> bp = new (buffer_pool) core::Buffer<uint8_t>(buffer_pool);
        CHECK(bp);

        bp.resize(funcs->payload_size_from_samples(num_samples));

        for (size_t n = 0; n < bp.size(); n++) {
            bp.data()[n] = 0;
        }

        return bp;
    }

    void encode(const core::Slice<uint8_t>& bp, const sample_t* samples, size_t offset,
                size_t num_samples, packet::channel_mask_t channels) {
        CHECK(funcs);

        UNSIGNED_LONGS_EQUAL(num_samples,
                             funcs->encode_samples(bp.data(), bp.size(), offset, samples,
                                                   num_samples, channels));
    }

    void decode(const core::Slice<uint8_t>& bp, size_t offset, size_t num_samples,
                packet::channel_mask_t channels) {
        CHECK(funcs);

        for (size_t i = 0; i < MaxSamples; i++) {
            output[i] = 0.0f;
        }

        UNSIGNED_LONGS_EQUAL(num_samples,
                             funcs->decode_samples(bp.data(), bp.size(), offset, output,
                                                   num_samples, channels));
    }

    void check(const sample_t* samples, size_t num_samples,
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

    use(PCM_int16_1ch);

    UNSIGNED_LONGS_EQUAL(NumSamples * 1 * sizeof(int16_t),
                         funcs->payload_size_from_samples(NumSamples));
}

TEST(pcm_funcs, payload_size_2ch) {
    enum { NumSamples = 77 };

    use(PCM_int16_2ch);

    UNSIGNED_LONGS_EQUAL(NumSamples * 2 * sizeof(int16_t),
                         funcs->payload_size_from_samples(NumSamples));
}

TEST(pcm_funcs, encode_decode_1ch) {
    enum { NumSamples = 5 };

    use(PCM_int16_1ch);

    core::Slice<uint8_t> bp = new_buffer(NumSamples);

    const sample_t samples[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    encode(bp, samples, 0, NumSamples, 0x1);
    decode(bp, 0, NumSamples, 0x1);

    check(samples, NumSamples, 0x1);
}

TEST(pcm_funcs, encode_decode_2ch) {
    enum { NumSamples = 5 };

    use(PCM_int16_2ch);

    core::Slice<uint8_t> bp = new_buffer(NumSamples);

    const sample_t samples[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(bp, samples, 0, NumSamples, 0x3);
    decode(bp, 0, NumSamples, 0x3);

    check(samples, NumSamples, 0x3);
}

TEST(pcm_funcs, encode_mask_subset) {
    enum { NumSamples = 5 };

    use(PCM_int16_2ch);

    core::Slice<uint8_t> bp = new_buffer(NumSamples);

    const sample_t input[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    encode(bp, input, 0, NumSamples, 0x2);
    decode(bp, 0, NumSamples, 0x3);

    const sample_t output[NumSamples * 2] = {
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

    use(PCM_int16_2ch);

    core::Slice<uint8_t> bp = new_buffer(NumSamples);

    const sample_t input[NumSamples * 3] = {
        -0.1f, 0.1f, 0.8f, //
        -0.2f, 0.2f, 0.8f, //
        -0.3f, 0.3f, 0.8f, //
        -0.4f, 0.4f, 0.8f, //
        -0.5f, 0.5f, 0.8f, //
    };

    encode(bp, input, 0, NumSamples, 0x7);
    decode(bp, 0, NumSamples, 0x3);

    const sample_t output[NumSamples * 2] = {
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

    use(PCM_int16_2ch);

    core::Slice<uint8_t> bp = new_buffer(NumSamples);

    const sample_t input[NumSamples * 3] = {
        -0.1f, 0.8f, //
        -0.2f, 0.8f, //
        -0.3f, 0.8f, //
        -0.4f, 0.8f, //
        -0.5f, 0.8f, //
    };

    encode(bp, input, 0, NumSamples, 0x5);
    decode(bp, 0, NumSamples, 0x3);

    const sample_t output[NumSamples * 2] = {
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

    use(PCM_int16_2ch);

    core::Slice<uint8_t> bp = new_buffer(NumSamples);

    const sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(bp, input, 0, NumSamples, 0x3);
    decode(bp, 0, NumSamples, 0x2);

    const sample_t output[NumSamples] = {
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

    use(PCM_int16_2ch);

    core::Slice<uint8_t> bp = new_buffer(NumSamples);

    const sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(bp, input, 0, NumSamples, 0x3);
    decode(bp, 0, NumSamples, 0x7);

    const sample_t output[NumSamples * 3] = {
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

    use(PCM_int16_2ch);

    core::Slice<uint8_t> bp = new_buffer(NumSamples);

    const sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(bp, input, 0, NumSamples, 0x3);
    decode(bp, 0, NumSamples, 0x6);

    const sample_t output[NumSamples * 2] = {
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

    use(PCM_int16_2ch);

    core::Slice<uint8_t> bp = new_buffer(NumSamples);

    const sample_t input1[(NumSamples - Off) * 2] = {
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(bp, input1, Off, NumSamples - Off, 0x3);

    const sample_t input2[Off] = {
        -0.1f, //
        -0.2f, //
    };

    encode(bp, input2, 0, Off, 0x1);

    const sample_t output[NumSamples * 2] = {
        -0.1f, 0.0f, //
        -0.2f, 0.0f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    decode(bp, 0, NumSamples, 0x3);

    check(output, NumSamples, 0x3);
}

TEST(pcm_funcs, decode_incremenal) {
    enum { NumSamples = 5, Off = 2 };

    use(PCM_int16_2ch);

    core::Slice<uint8_t> bp = new_buffer(NumSamples);

    const sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(bp, input, 0, NumSamples, 0x3);

    decode(bp, 0, Off, 0x3);

    const sample_t output1[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
    };

    check(output1, Off, 0x3);

    decode(bp, Off, NumSamples - Off, 0x1);

    const sample_t output2[NumSamples] = {
        -0.3f, //
        -0.4f, //
        -0.5f, //
    };

    check(output2, NumSamples - Off, 0x1);

    decode(bp, Off, NumSamples - Off, 0x2);

    const sample_t output3[NumSamples] = {
        0.3f, //
        0.4f, //
        0.5f, //
    };

    check(output3, NumSamples - Off, 0x2);
}

TEST(pcm_funcs, encode_truncate) {
    enum { NumSamples = 5, Off = 2 };

    use(PCM_int16_2ch);

    core::Slice<uint8_t> bp = new_buffer(NumSamples);

    const sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    UNSIGNED_LONGS_EQUAL(
        NumSamples - Off,
        funcs->encode_samples(bp.data(), bp.size(), Off, input, NumSamples, 0x3));

    UNSIGNED_LONGS_EQUAL(
        0, funcs->encode_samples(bp.data(), bp.size(), 123, input, NumSamples, 0x3));

    const sample_t output[NumSamples * 2] = {
        0.0f,  0.0f, //
        0.0f,  0.0f, //
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
    };

    decode(bp, 0, NumSamples, 0x3);

    check(output, NumSamples, 0x3);
}

TEST(pcm_funcs, decode_truncate) {
    enum { NumSamples = 5, Off = 2 };

    use(PCM_int16_2ch);

    core::Slice<uint8_t> bp = new_buffer(NumSamples);

    const sample_t input[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    encode(bp, input, 0, NumSamples, 0x3);

    UNSIGNED_LONGS_EQUAL(
        NumSamples - Off,
        funcs->decode_samples(bp.data(), bp.size(), Off, output, NumSamples, 0x3));

    UNSIGNED_LONGS_EQUAL(
        0, funcs->decode_samples(bp.data(), bp.size(), 123, output, NumSamples, 0x3));

    const sample_t output[NumSamples * 2] = {
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
        0.0f,  0.0f, //
        0.0f,  0.0f, //
    };

    check(output, NumSamples, 0x3);
}

} // namespace audio
} // namespace roc
