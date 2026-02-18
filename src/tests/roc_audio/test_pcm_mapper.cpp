/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include <stdio.h>

#include "roc_audio/pcm_mapper.h"
#include "roc_audio/sample.h"
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/print_memory.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

const double Epsilon = 0.000001;

void map(const void* input,
         void* output,
         size_t in_bytes,
         size_t out_bytes,
         size_t n_samples,
         PcmSubformat in_fmt,
         PcmSubformat out_fmt) {
    PcmMapper mapper(in_fmt, out_fmt);

    UNSIGNED_LONGS_EQUAL(n_samples, mapper.input_sample_count(in_bytes));
    UNSIGNED_LONGS_EQUAL(in_bytes, mapper.input_byte_count(n_samples));

    UNSIGNED_LONGS_EQUAL(n_samples, mapper.output_sample_count(out_bytes));
    UNSIGNED_LONGS_EQUAL(out_bytes, mapper.output_byte_count(n_samples));

    size_t in_off = 0;
    size_t out_off = 0;

    const size_t actual_samples =
        mapper.map(input, in_bytes, in_off, output, out_bytes, out_off, n_samples);

    UNSIGNED_LONGS_EQUAL(n_samples, actual_samples);

    UNSIGNED_LONGS_EQUAL(in_bytes * 8, in_off);
    UNSIGNED_LONGS_EQUAL(out_bytes * 8, out_off);
}

template <class T> void report(const T* expected, const T* actual, size_t n_items) {
    printf("\n");

    printf("expected:\n");
    core::print_memory(expected, n_items);

    printf("actual:\n");
    core::print_memory(actual, n_items);
}

template <class T> void compare(const T* expected, const T* actual, size_t n_items) {
    for (size_t n = 0; n < n_items; n++) {
        if (expected[n] != actual[n]) {
            report(expected, actual, n_items);

            LONGS_EQUAL(expected[n], actual[n]);
        }
    }
}

void compare(const float* expected, const float* actual, size_t n_items) {
    for (size_t n = 0; n < n_items; n++) {
        if ((double)std::abs(expected[n] - actual[n]) > Epsilon) {
            report(expected, actual, n_items);

            DOUBLES_EQUAL(expected[n], actual[n], Epsilon);
        }
    }
}

void compare(const double* expected, const double* actual, size_t n_items) {
    for (size_t n = 0; n < n_items; n++) {
        if (std::abs(expected[n] - actual[n]) > Epsilon) {
            report(expected, actual, n_items);

            DOUBLES_EQUAL(expected[n], actual[n], Epsilon);
        }
    }
}

} // namespace

TEST_GROUP(pcm_mapper) {};

TEST(pcm_mapper, raw_to_raw) {
    sample_t input[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 1.0f,
    };
    sample_t expected_output[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 1.0f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_int8) {
    sample_t input[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 0.992187f, 0.992188f, 1.0f,
    };
    int8_t expected_output[] = {
        -128, -39, 0, 39, 126, 127, 127,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int8_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_SInt8);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int8_to_raw) {
    int8_t input[] = {
        -128, -39, 0, 39, 127,
    };
    sample_t expected_output[] = {
        -1.0f, -0.304688f, 0.0f, 0.304688f, 0.992188f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_SInt8, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_int16) {
    sample_t input[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 0.999969f, 0.999970f, 1.0f,
    };
    int16_t expected_output[] = {
        -32768, -10000, 0, 10000, 32766, 32767, 32767,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int16_to_raw) {
    int16_t input[] = {
        -32768, -10000, 0, 10000, 32767,
    };
    sample_t expected_output[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 0.999969f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_SInt16, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_int32) {
    sample_t input[] = {
        -1.0f,      //
        -0.305176f, //
        0.0f,       //
        0.305176f,  //
        0.999999f,  //
        1.0f,       //
    };
    int32_t expected_output[] = {
        -2147483648LL, // -1.0
        -655360448,    // -0.305176
        0,             // 0
        655360448,     // 0.305176
        2147481472,    // 0.999999
        2147483647,    // 1.0
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int32_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_SInt32);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int32_to_raw) {
    int32_t input[] = {
        -2147483648LL, // -1
        -655360000,    // -0.305176
        0,             // 0
        655360000,     // 0.305176
        2147482559,    // last before clip
        2147482560,    // clip
        2147483647     // also clip
    };
    sample_t expected_output[] = {
        -1.0f,      //
        -0.305176f, //
        0.0f,       //
        0.305176f,  //
        0.999999f,  //
        1.0f,       //
        1.0f        //
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    float actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_SInt32, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_int64) {
    sample_t input[] = {
        -1.0f,      //
        -0.305176f, //
        0.0f,       //
        0.305176f,  //
        1.0f,       //
    };
    int64_t expected_output[] = {
        -9223372036854775807LL - 1, // -1.0
        -2814751691251908608LL,     // -0.305176
        0,                          // 0
        2814751691251908608LL,      // 0.305176
        9223372036854775807LL,      // 1.0
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int64_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_SInt64);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int64_to_raw) {
    int64_t input[] = {
        -9223372036854775807LL - 1, // -1.0
        -2814749767106560000LL,     // -0.305176
        0,                          // 0
        2814749767106560000LL,      // 0.305176
        9223372036854775807LL,      // 1.0
    };
    sample_t expected_output[] = {
        -1.0f,      //
        -0.305176f, //
        0.0f,       //
        0.305176f,  //
        1.0f,       //
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_SInt64, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_float32) {
    sample_t input[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 1.0f,
    };
    float expected_output[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 1.0f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    float actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_Float32);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, float32_to_raw) {
    float input[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 1.0f,
    };
    sample_t expected_output[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 1.0f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Float32, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_float64) {
    sample_t input[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 1.0f,
    };
    double expected_output[] = {
        -1.0, -0.305176, 0.0, 0.305176, 1.0,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    double actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_Float64);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, float64_to_raw) {
    double input[] = {
        -1.0, -0.305176, 0.0, 0.305176, 1.0,
    };
    sample_t expected_output[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 1.0f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Float64, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_uint16) {
    sample_t input[] = {
        -1.0f,      //
        -0.305176f, //
        0.0f,       //
        0.305176f,  //
        0.999969f,  //
        0.999970f,  //
        1.0f,       //
    };
    uint16_t expected_output[] = {
        0,     // -1.0
        22768, // -0.305176
        32768, // 0
        42768, // 0.305176
        65534, // 0.999969
        65535, // 0.999970
        65535, // 1.0
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    uint16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_UInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, uint16_to_raw) {
    uint16_t input[] = {
        0,     // -1.0
        22768, // -0.305176
        32768, // 0
        42768, // 0.305176
        65535, // 0.999969
    };
    sample_t expected_output[] = {
        -1.0f,      //
        -0.305176f, //
        0.0f,       //
        0.305176f,  //
        0.999969f,  //
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_UInt16, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_uint32) {
    sample_t input[] = {
        -1.0f,      //
        -0.305176f, //
        0.0f,       //
        0.305176f,  //
        0.999999f,  //
        1.0f,       //
    };
    uint32_t expected_output[] = {
        0u,          // -1.0
        1492123200u, // -0.305176
        2147483648u, // 0
        2802844096u, // 0.305176
        4294965120u, // 0.999999
        4294967295u, // 1.0
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    uint32_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_UInt32);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, uint32_to_raw) {
    uint32_t input[] = {
        0u,          // -1.0
        1492123200u, // -0.305176
        2147483648u, // 0
        2802844096u, // 0.305176
        4294965183u, // 0.999999
        4294965184u, // 1.0
        4294967295u, // 1.0
    };
    sample_t expected_output[] = {
        -1.0f,      //
        -0.305176f, //
        0.0f,       //
        0.305176f,  //
        0.999999f,  //
        1.0f,       //
        1.0f,       //
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_UInt32, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_int16be) {
    sample_t input[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 1.0f,
    };
    uint8_t expected_output[] = {
        0x80, 0x00, // -32768
        0xd8, 0xf0, // -10000
        0x00, 0x00, // 0
        0x27, 0x10, // 10000
        0x7f, 0xff, // 32767
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_SInt16_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, raw_to_int16le) {
    sample_t input[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 1.0f,
    };
    uint8_t expected_output[] = {
        0x00, 0x80, // -32768
        0xf0, 0xd8, // -10000
        0x00, 0x00, // 0
        0x10, 0x27, // 10000
        0xff, 0x7f, // 32767
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_SInt16_Le);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int16be_to_raw) {
    uint8_t input[] = {
        0x80, 0x00, // -32768
        0xd8, 0xf0, // -10000
        0x00, 0x00, // 0
        0x27, 0x10, // 10000
        0x7f, 0xff, // 32767
    };
    sample_t expected_output[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 0.999969f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_SInt16_Be, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int16le_to_raw) {
    uint8_t input[] = {
        0x00, 0x80, // -32768
        0xf0, 0xd8, // -10000
        0x00, 0x00, // 0
        0x10, 0x27, // 10000
        0xff, 0x7f, // 32767
    };
    sample_t expected_output[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 0.999969f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_SInt16_Le, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_int18b4be) {
    sample_t input[] = {
        -1.0f, -0.305176f, 0.0f, 0.999939f, 1.0f,
    };
    uint8_t expected_output[] = {
        0x00, 0x02, 0x00, 0x00, // -131072
        0x00, 0x03, 0x63, 0xc0, // -40000
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x01, 0xff, 0xf8, // 131064
        0x00, 0x01, 0xff, 0xff, // 131071
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_SInt18_4_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int18b4be_to_raw) {
    uint8_t input[] = {
        0x00, 0x02, 0x00, 0x00, // -131072
        0x00, 0x03, 0x63, 0xc0, // -40000
        0x00, 0x03, 0xd8, 0xf0, // -10000
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x00, 0x27, 0x10, // 10000
        0x00, 0x00, 0x9c, 0x40, // 40000
        0x00, 0x01, 0xff, 0xf8, // 131064
        0x00, 0x01, 0xff, 0xff, // 131071
    };
    sample_t expected_output[] = {
        -1.0f, -0.305176f, -0.076294f, 0.0f, 0.076294f, 0.305176f, 0.999939f, 0.999992f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_SInt18_4_Be, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_int20b3be) {
    sample_t input[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 1.0f,
    };
    uint8_t expected_output[] = {
        0x08, 0x00, 0x00, // -524288
        0x0d, 0x8f, 0x00, // -160000,
        0x00, 0x00, 0x00, // 0
        0x02, 0x71, 0x00, // 160000,
        0x07, 0xff, 0xff, // 524287,
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_SInt20_3_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int20b3be_to_raw) {
    uint8_t input[] = {
        0x08, 0x00, 0x00, // -524288
        0x0d, 0x8f, 0x00, // -160000,
        0x00, 0x00, 0x00, // 0
        0x02, 0x71, 0x00, // 160000,
        0x07, 0xff, 0xf0, // 524272,
    };
    sample_t expected_output[] = {
        -1.0f, -0.305176f, 0.0f, 0.305176f, 0.999969f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_SInt20_3_Be, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_int20b4be) {
    sample_t input[] = {
        -1.0f, -0.305176f, 0.0f, 0.999939f, 1.0f,
    };
    uint8_t expected_output[] = {
        0x00, 0x08, 0x00, 0x00, // -524288
        0x00, 0x0d, 0x8f, 0x00, // -160000
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x07, 0xff, 0xe0, // 524256
        0x00, 0x07, 0xff, 0xff, // 524287
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_SInt20_4_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int20b4be_to_raw) {
    uint8_t input[] = {
        0x00, 0x08, 0x00, 0x00, // -524288
        0x00, 0x0d, 0x8f, 0x00, // -160000
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x07, 0xff, 0xe0, // 524256
        0x00, 0x07, 0xff, 0xff, // 524287
    };
    sample_t expected_output[] = {
        -1.0f, -0.305176f, 0.0f, 0.999939f, 0.999998f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_SInt20_4_Be, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_int24be) {
    sample_t input[] = {
        -1.0f, -0.3051758f, 0.0f, 0.3051758f, 1.0f,
    };
    uint8_t expected_output[] = {
        0x80, 0x00, 0x00, // -8388608
        0xd8, 0xf0, 0x00, // -2560000
        0x00, 0x00, 0x00, // 0
        0x27, 0x10, 0x00, // 2560000
        0x7f, 0xff, 0xff, // 8388607
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_SInt24_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int24be_to_raw) {
    uint8_t input[] = {
        0x80, 0x00, 0x00, // -8388608
        0xd8, 0xf0, 0x00, // -2560000,
        0x00, 0x00, 0x00, // 0
        0x27, 0x10, 0x00, // 2560000,
        0x7f, 0xff, 0x00, // 8388352,
    };
    sample_t expected_output[] = {
        -1.0f, -0.3051758f, 0.0f, 0.3051758f, 0.999969f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_SInt24_Be, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_int24b4be) {
    sample_t input[] = {
        -1.0f, -0.3051758f, 0.0f, 0.999939f, 1.0f,
    };
    uint8_t expected_output[] = {
        0x00, 0x80, 0x00, 0x00, // -8388608
        0x00, 0xd8, 0xf0, 0x00, // -2560000
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x7f, 0xfe, 0x00, // 8388096
        0x00, 0x7f, 0xff, 0xff, // 8388607
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_SInt24_4_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int24b4be_to_raw) {
    uint8_t input[] = {
        0x00, 0x80, 0x00, 0x00, // -8388608
        0x00, 0xd8, 0xf0, 0x00, // -2560000
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x7f, 0xfe, 0x00, // 8388096
        0x00, 0x7f, 0xff, 0xff, // 8388352
    };
    sample_t expected_output[] = {
        -1.0f, -0.3051758f, 0.0f, 0.999939f, 1.0f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_SInt24_4_Be, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, raw_to_int20be) {
    sample_t input[] = {
        -0.931322f,
        -0.465660f,
        0.465660f,
        0.931322f,
    };
    uint8_t expected_output[] = {
        // -488280 (0x88ca8), -244139 (0xc4655)
        0x88,
        0xca,
        0x8c,
        0x46,
        0x55,
        // 244140 (0x3b9ab), 488280 (0x77358)
        0x3b,
        0x9a,
        0xb7,
        0x73,
        0x58,
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_Raw, PcmSubformat_SInt20_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int20be_to_raw) {
    uint8_t input[] = {
        // -488280 (0x88caf), -244139 (0xc4655)
        0x88,
        0xca,
        0x8c,
        0x46,
        0x55,
        // 244140 (0x3b9ab), 488280 (0x77358)
        0x3b,
        0x9a,
        0xb7,
        0x73,
        0x58,
    };
    sample_t expected_output[] = {
        -0.931320f,
        -0.465658f,
        0.465658f,
        0.931320f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    sample_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmSubformat_SInt20_Be, PcmSubformat_Raw);

    compare(expected_output, actual_output, NumSamples);
}

} // namespace audio
} // namespace roc
