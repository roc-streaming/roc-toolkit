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
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/print_buffer.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

const double Epsilon = 0.0001;

void map(const void* input,
         void* output,
         size_t in_bytes,
         size_t out_bytes,
         size_t n_samples,
         PcmFormat in_fmt,
         PcmFormat out_fmt) {
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
    core::print_buffer(expected, n_items);

    printf("actual:\n");
    core::print_buffer(actual, n_items);
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

TEST(pcm_mapper, int16_to_int16) {
    int16_t input[] = {
        -32768, -10000, 0, 10000, 32767,
    };
    int16_t expected_output[] = {
        -32768, -10000, 0, 10000, 32767,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int16_to_int8) {
    int16_t input[] = {
        -32768, -10000, 0, 10000, 32767,
    };
    int8_t expected_output[] = {
        -128, -39, 0, 39, 127,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int8_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16, PcmFormat_SInt8);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int8_to_int16) {
    int8_t input[] = {
        -128, -39, 0, 39, 127,
    };
    int16_t expected_output[] = {
        -32768, -9984, 0, 9984, 32512,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt8, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int16_to_int32) {
    int16_t input[] = {
        -32768, -10000, 0, 10000, 32767,
    };
    int32_t expected_output[] = {
        -2147483648ll, -655360000, 0, 655360000, 2147418112,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int32_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16, PcmFormat_SInt32);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int32_to_int16) {
    int32_t input[] = {
        -2147483648ll, //
        -655360000,    //
        0,             //
        32767,         // 0
        32768,         // 1
        2147385343,    // 32766
        2147385344,    // 32767
        2147450879,    // last before clip
        2147450880,    // clip
        2147450881,    // also clip
        2147483647     // also clip
    };
    int16_t expected_output[] = {
        -32768, -10000, 0, 0, 1, 32766, 32767, 32767, 32767, 32767, 32767,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt32, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int16_to_int64) {
    int16_t input[] = {
        -32768, //
        -10000, //
        0,      //
        10000,  //
        32767,  //
    };
    int64_t expected_output[] = {
        -9223372036854775807ll - 1, //
        -2814749767106560000ll,     //
        0,                          //
        2814749767106560000ll,      //
        9223090561878065152ll,      //
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int64_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16, PcmFormat_SInt64);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int64_to_int16) {
    int64_t input[] = {
        -9223372036854775807ll - 1, //
        -2814749767106560000ll,     //
        0,                          //
        2814749767106560000ll,      //
        9223372036854775807ll,      //
    };
    int16_t expected_output[] = {
        -32768, //
        -10000, //
        0,      //
        10000,  //
        32767,  //
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt64, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int16_to_float32) {
    int16_t input[] = {
        -32768, -10000, 0, 32766, 32767,
    };
    float expected_output[] = {
        -1.0f, -0.305176f, 0.0f, 0.999939f, 0.999969f,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    float actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16, PcmFormat_Float32);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, float32_to_int16) {
    float input[] = {
        -1.0f, -0.305176f, 0.0f, 0.999939f, 0.999969f, 1.0f,
    };
    int16_t expected_output[] = {
        -32768, -10000, 0, 32766, 32766, 32767,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_Float32, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int32_to_float32) {
    int32_t input[] = {
        -2147483648ll, // -1
        -655360000,    // -0.305176
        0,             // 0
        32767,         // 0.000015
        2147482559,    // last before clip
        2147482560,    // clip
        2147483647     // also clip
    };
    float expected_output[] = {
        -1.0f,      //
        -0.305176f, //
        0.0f,       //
        0.000015f,  //
        0.999999f,  //
        1.0f,       //
        1.0f        //
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    float actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt32, PcmFormat_Float32);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, float32_to_int32) {
    float input[] = {
        -1.0f,      //
        -0.305176f, //
        0.0f,       //
        0.000015f,  //
        0.999999f,  //
        1.0f,       //
    };
    int32_t expected_output[] = {
        -2147483648ll, //
        -655360448,    //
        0,             //
        32212,         //
        2147481472,    //
        2147483647,    //
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int32_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_Float32, PcmFormat_SInt32);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int32_to_float64) {
    int32_t input[] = {
        -2147483648ll, // -1
        -655360000,    // -0.305176
        0,             // 0
        32767,         // 0.000015
        2147482559,    // last before clip
        2147482560,    // clip
        2147483647     // also clip
    };
    double expected_output[] = {
        -1.0,      //
        -0.305176, //
        0.0,       //
        0.000015,  //
        0.999999,  //
        1.0,       //
        1.0        //
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    double actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt32, PcmFormat_Float64);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, float64_to_int32) {
    double input[] = {
        -1.0,      //
        -0.305176, //
        0.0,       //
        0.000015,  //
        0.999999,  //
        1.0,       //
    };
    int32_t expected_output[] = {
        -2147483648ll, //
        -655360469,    //
        0,             //
        32212,         //
        2147481500,    //
        2147483647,    //
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int32_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_Float64, PcmFormat_SInt32);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, uint16_to_uint32) {
    uint16_t input[] = {
        0,
        10000,
        65535,
    };
    uint32_t expected_output[] = {
        0,
        655360000,
        4294901760u,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    uint32_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_UInt16, PcmFormat_UInt32);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, uint32_to_uint16) {
    uint32_t input[] = {
        0,
        655360000,
        4294901760u,
        4294967295u,
    };
    uint16_t expected_output[] = {
        0,
        10000,
        65535,
        65535,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    uint16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_UInt32, PcmFormat_UInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, uint16_to_int32) {
    uint16_t input[] = {
        0,
        10000,
        65535,
    };
    int32_t expected_output[] = {
        -2147483648ll,
        -1492123648,
        2147418112,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int32_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_UInt16, PcmFormat_SInt32);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int16_to_uint32) {
    int16_t input[] = {
        -32768,
        0,
        10000,
        32767,
    };
    uint32_t expected_output[] = {
        0,
        2147483648u,
        2802843648u,
        4294901760u,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    uint32_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16, PcmFormat_UInt32);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, uint32_to_int16) {
    uint32_t input[] = {
        0u, 2147483648u, 2802843648u, 4294901760u, 4294967295u,
    };
    int16_t expected_output[] = {
        -32768, 0, 10000, 32767, 32767,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_UInt32, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int32_to_uint16) {
    int32_t input[] = {
        -2147483648ll,
        -1492123648,
        2147418112,
        2147483647,
    };
    uint16_t expected_output[] = {
        0,
        10000,
        65535,
        65535,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(input) };

    uint16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt32, PcmFormat_UInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int16_to_int18b4) {
    int16_t input[] = {
        -32768, -10000, 0, 32766, 32767,
    };
    uint8_t expected_output[] = {
        0x00, 0x02, 0x00, 0x00, // -131072
        0x00, 0x03, 0x63, 0xc0, // -40000
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x01, 0xff, 0xf8, // 131064
        0x00, 0x01, 0xff, 0xfc, // 131068
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16, PcmFormat_SInt18_4_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int18b4_to_int16) {
    uint8_t input[] = {
        0x00, 0x02, 0x00, 0x00, // -131072
        0x00, 0x03, 0x63, 0xc0, // -40000
        0x00, 0x03, 0xd8, 0xf0, // -10000
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x00, 0x27, 0x10, // 10000
        0x00, 0x00, 0x9c, 0x40, // 40000
        0x00, 0x01, 0xff, 0xf8, // 131064
        0x00, 0x01, 0xff, 0xfc, // 131068
    };
    int16_t expected_output[] = {
        -32768, -10000, -2500, 0, 2500, 10000, 32766, 32767,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt18_4_Be, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int16_to_int20b4) {
    int16_t input[] = {
        -32768, -10000, 0, 32766, 32767,
    };
    uint8_t expected_output[] = {
        0x00, 0x08, 0x00, 0x00, // -524288
        0x00, 0x0d, 0x8f, 0x00, // -160000
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x07, 0xff, 0xe0, // 524256
        0x00, 0x07, 0xff, 0xf0, // 524272
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16, PcmFormat_SInt20_4_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int20b4_to_int16) {
    uint8_t input[] = {
        0x00, 0x08, 0x00, 0x00, // -524288
        0x00, 0x0d, 0x8f, 0x00, // -160000
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x07, 0xff, 0xe0, // 524256
        0x00, 0x07, 0xff, 0xff, // 524287
    };
    int16_t expected_output[] = {
        -32768, -10000, 0, 32766, 32767,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt20_4_Be, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int16_to_int24b4) {
    int16_t input[] = {
        -32768, -10000, 0, 32766, 32767,
    };
    uint8_t expected_output[] = {
        0x00, 0x80, 0x00, 0x00, // -8388608
        0x00, 0xd8, 0xf0, 0x00, // -2560000
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x7f, 0xfe, 0x00, // 8388096
        0x00, 0x7f, 0xff, 0x00, // 8388352
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16, PcmFormat_SInt24_4_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int24b4_to_int16) {
    uint8_t input[] = {
        0x00, 0x80, 0x00, 0x00, // -8388608
        0x00, 0xd8, 0xf0, 0x00, // -2560000
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x7f, 0xfe, 0x00, // 8388096
        0x00, 0x7f, 0xff, 0xff, // 8388352
    };
    int16_t expected_output[] = {
        -32768, -10000, 0, 32766, 32767,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt24_4_Be, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, uint24b4_to_int20b4) {
    uint8_t input[] = {
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x4c, 0x4b, 0x40, // 5000000
        0x00, 0x89, 0x54, 0x40, // 9000000
        0x00, 0xff, 0xff, 0xff, // 16777215
    };
    uint8_t expected_output[] = {
        0x00, 0x08, 0x00, 0x00, // -524288
        0x00, 0x0c, 0xc4, 0xb4, // -211788
        0x00, 0x00, 0x95, 0x44, // 38212
        0x00, 0x07, 0xff, 0xff, // 524287
    };

    enum { NumBytes = ROC_ARRAY_SIZE(input), NumSamples = NumBytes / 4 };

    uint8_t actual_output[NumBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_UInt24_4_Be, PcmFormat_SInt20_4_Be);

    compare(expected_output, actual_output, NumBytes);
}

TEST(pcm_mapper, int20b4_to_uint24b4) {
    uint8_t input[] = {
        0x00, 0x08, 0x00, 0x00, // -524288
        0x00, 0x0c, 0xc4, 0xb4, // -211788
        0x00, 0x00, 0x95, 0x44, // 38212
        0x00, 0x07, 0xff, 0xff, // 524287
    };
    uint8_t expected_output[] = {
        0x00, 0x00, 0x00, 0x00, // 0
        0x00, 0x4c, 0x4b, 0x40, // 5000000
        0x00, 0x89, 0x54, 0x40, // 9000000
        0x00, 0xff, 0xff, 0xf0, // 16777200
    };

    enum { NumBytes = ROC_ARRAY_SIZE(input), NumSamples = NumBytes / 4 };

    uint8_t actual_output[NumBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt20_4_Be, PcmFormat_UInt24_4_Be);

    compare(expected_output, actual_output, NumBytes);
}

TEST(pcm_mapper, int32_to_int20) {
    int32_t input[] = {
        -2000000000,
        -1000000000,
        1000000000,
        2000000000,
    };
    uint8_t expected_output[] = {
        // -488281 (0x88ca7), -244140 (0xc4653)
        0x88,
        0xca,
        0x7c,
        0x46,
        0x53,
        // 244140 (0x3b9ad), 488281 (0x77359)
        0x3b,
        0x9a,
        0xd7,
        0x73,
        0x59,
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt32, PcmFormat_SInt20_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int20_to_int32) {
    uint8_t input[] = {
        // -488281 (0x88ca7), -244140 (0xc4653)
        0x88,
        0xca,
        0x7c,
        0x46,
        0x53,
        // 244140 (0x3b9ad), 488281 (0x77359)
        0x3b,
        0x9a,
        0xd7,
        0x73,
        0x59,
    };
    int32_t expected_output[] = {
        -1999998976,
        -1000001536,
        1000001536,
        1999998976,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    int32_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt20_Be, PcmFormat_SInt32);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int16_to_int20b3) {
    int16_t input[] = {
        -32768, -10000, 0, 10000, 32767,
    };
    uint8_t expected_output[] = {
        0x08, 0x00, 0x00, // -524288
        0x0d, 0x8f, 0x00, // -160000,
        0x00, 0x00, 0x00, // 0
        0x02, 0x71, 0x00, // 160000,
        0x07, 0xff, 0xf0, // 524272,
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16, PcmFormat_SInt20_3_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int20b3_to_int16) {
    uint8_t input[] = {
        0x08, 0x00, 0x00, // -524288
        0x0d, 0x8f, 0x00, // -160000,
        0x00, 0x00, 0x00, // 0
        0x02, 0x71, 0x00, // 160000,
        0x07, 0xff, 0xf0, // 524272,
    };
    int16_t expected_output[] = {
        -32768, -10000, 0, 10000, 32767,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt20_3_Be, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, int16_to_int24) {
    int16_t input[] = {
        -32768, -10000, 0, 10000, 32767,
    };
    uint8_t expected_output[] = {
        0x80, 0x00, 0x00, // -8388608
        0xd8, 0xf0, 0x00, // -2560000,
        0x00, 0x00, 0x00, // 0
        0x27, 0x10, 0x00, // 2560000,
        0x7f, 0xff, 0x00, // 8388352,
    };

    enum {
        NumSamples = ROC_ARRAY_SIZE(input),
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output)
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16, PcmFormat_SInt24_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, int24_to_int16) {
    uint8_t input[] = {
        0x80, 0x00, 0x00, // -8388608
        0xd8, 0xf0, 0x00, // -2560000,
        0x00, 0x00, 0x00, // 0
        0x27, 0x10, 0x00, // 2560000,
        0x7f, 0xff, 0x00, // 8388352,
    };
    int16_t expected_output[] = {
        -32768, -10000, 0, 10000, 32767,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt24_Be, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, native_to_be) {
    int16_t input[] = {
        -32768, -10000, 0, 10000, 32767,
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
        PcmFormat_SInt16, PcmFormat_SInt16_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, native_to_le) {
    int16_t input[] = {
        -32768, -10000, 0, 10000, 32767,
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
        PcmFormat_SInt16, PcmFormat_SInt16_Le);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, be_to_native) {
    uint8_t input[] = {
        0x80, 0x00, // -32768
        0xd8, 0xf0, // -10000
        0x00, 0x00, // 0
        0x27, 0x10, // 10000
        0x7f, 0xff, // 32767
    };
    int16_t expected_output[] = {
        -32768, -10000, 0, 10000, 32767,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16_Be, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, le_to_native) {
    uint8_t input[] = {
        0x00, 0x80, // -32768
        0xf0, 0xd8, // -10000
        0x00, 0x00, // 0
        0x10, 0x27, // 10000
        0xff, 0x7f, // 32767
    };
    int16_t expected_output[] = {
        -32768, -10000, 0, 10000, 32767,
    };

    enum { NumSamples = ROC_ARRAY_SIZE(expected_output) };

    int16_t actual_output[NumSamples] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16_Le, PcmFormat_SInt16);

    compare(expected_output, actual_output, NumSamples);
}

TEST(pcm_mapper, be_to_le) {
    uint8_t input[] = {
        0x80, 0x00, // -32768
        0xd8, 0xf0, // -10000
        0x00, 0x00, // 0
        0x27, 0x10, // 10000
        0x7f, 0xff, // 32767
    };
    uint8_t expected_output[] = {
        0x00, 0x80, // -32768
        0xf0, 0xd8, // -10000
        0x00, 0x00, // 0
        0x10, 0x27, // 10000
        0xff, 0x7f, // 32767
    };

    enum {
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output),
        NumSamples = NumOutputBytes / 2
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16_Be, PcmFormat_SInt16_Le);

    compare(expected_output, actual_output, NumOutputBytes);
}

TEST(pcm_mapper, le_to_be) {
    uint8_t input[] = {
        0x00, 0x80, // -32768
        0xf0, 0xd8, // -10000
        0x00, 0x00, // 0
        0x10, 0x27, // 10000
        0xff, 0x7f, // 32767
    };
    uint8_t expected_output[] = {
        0x80, 0x00, // -32768
        0xd8, 0xf0, // -10000
        0x00, 0x00, // 0
        0x27, 0x10, // 10000
        0x7f, 0xff, // 32767
    };

    enum {
        NumOutputBytes = ROC_ARRAY_SIZE(expected_output),
        NumSamples = NumOutputBytes / 2
    };

    uint8_t actual_output[NumOutputBytes] = {};

    map(input, actual_output, sizeof(input), sizeof(actual_output), NumSamples,
        PcmFormat_SInt16_Le, PcmFormat_SInt16_Be);

    compare(expected_output, actual_output, NumOutputBytes);
}

} // namespace audio
} // namespace roc
