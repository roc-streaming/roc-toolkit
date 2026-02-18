/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/pcm_mapper.h"
#include "roc_audio/sample.h"
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"

#include "test_samples/pcm_float32_be.h"
#include "test_samples/pcm_float32_le.h"
#include "test_samples/pcm_sint16_be.h"
#include "test_samples/pcm_sint16_le.h"
#include "test_samples/pcm_sint24_be.h"
#include "test_samples/pcm_sint24_le.h"
#include "test_samples/pcm_sint32_be.h"
#include "test_samples/pcm_sint32_le.h"
#include "test_samples/pcm_sint8_be.h"
#include "test_samples/pcm_sint8_le.h"
#include "test_samples/pcm_uint16_be.h"
#include "test_samples/pcm_uint16_le.h"
#include "test_samples/pcm_uint24_be.h"
#include "test_samples/pcm_uint24_le.h"
#include "test_samples/pcm_uint32_be.h"
#include "test_samples/pcm_uint32_le.h"
#include "test_samples/pcm_uint8_be.h"
#include "test_samples/pcm_uint8_le.h"

namespace roc {
namespace audio {

namespace {

const double Epsilon = 0.01;

const test::SampleInfo* test_samples[] = {
    &test::sample_pcm_float32_be, &test::sample_pcm_float32_le,
    &test::sample_pcm_sint16_be,  &test::sample_pcm_sint16_le,
    &test::sample_pcm_sint24_be,  &test::sample_pcm_sint24_le,
    &test::sample_pcm_sint32_be,  &test::sample_pcm_sint32_le,
    &test::sample_pcm_sint8_be,   &test::sample_pcm_sint8_le,
    &test::sample_pcm_uint16_be,  &test::sample_pcm_uint16_le,
    &test::sample_pcm_uint24_be,  &test::sample_pcm_uint24_le,
    &test::sample_pcm_uint32_be,  &test::sample_pcm_uint32_le,
    &test::sample_pcm_uint8_be,   &test::sample_pcm_uint8_le,
};

} // namespace

TEST_GROUP(pcm_samples) {};

TEST(pcm_samples, decode) {
    for (size_t idx = 0; idx < ROC_ARRAY_SIZE(test_samples); idx++) {
        roc_log(LogDebug, "mapping %s to raw samples", test_samples[idx]->name);

        PcmSubformat in_fmt = test_samples[idx]->format;
        PcmSubformat out_fmt = PcmSubformat_Raw;

        PcmMapper mapper(in_fmt, out_fmt);

        UNSIGNED_LONGS_EQUAL(test_samples[idx]->num_samples,
                             mapper.input_sample_count(test_samples[idx]->num_bytes));

        UNSIGNED_LONGS_EQUAL(test_samples[idx]->num_bytes,
                             mapper.input_byte_count(test_samples[idx]->num_samples));

        sample_t decoded_samples[test::SampleInfo::MaxSamples] = {};

        const size_t in_bytes = test_samples[idx]->num_bytes;
        const size_t out_bytes = test_samples[idx]->num_samples * sizeof(sample_t);

        size_t in_off = 0;
        size_t out_off = 0;

        const size_t actual_samples =
            mapper.map(test_samples[idx]->bytes, in_bytes, in_off, decoded_samples,
                       out_bytes, out_off, test_samples[idx]->num_samples);

        UNSIGNED_LONGS_EQUAL(test_samples[idx]->num_samples, actual_samples);

        UNSIGNED_LONGS_EQUAL(in_bytes * 8, in_off);
        UNSIGNED_LONGS_EQUAL(out_bytes * 8, out_off);

        roc_log(LogDebug, "comparing samples");

        for (size_t n = 0; n < test_samples[idx]->num_samples; n++) {
            DOUBLES_EQUAL(test_samples[idx]->samples[n], decoded_samples[n], Epsilon);
        }
    }
}

TEST(pcm_samples, encode_decode) {
    for (size_t idx = 0; idx < ROC_ARRAY_SIZE(test_samples); idx++) {
        uint8_t encoded_samples[test::SampleInfo::MaxBytes] = {};
        sample_t decoded_samples[test::SampleInfo::MaxSamples] = {};

        { // encode
            roc_log(LogDebug, "mapping raw samples to %s", test_samples[idx]->name);

            PcmSubformat in_fmt = PcmSubformat_Raw;
            PcmSubformat out_fmt = test_samples[idx]->format;

            PcmMapper mapper(in_fmt, out_fmt);

            UNSIGNED_LONGS_EQUAL(
                test_samples[idx]->num_samples,
                mapper.output_sample_count(test_samples[idx]->num_bytes));

            UNSIGNED_LONGS_EQUAL(
                test_samples[idx]->num_bytes,
                mapper.output_byte_count(test_samples[idx]->num_samples));

            const size_t in_bytes = test_samples[idx]->num_samples * sizeof(sample_t);
            const size_t out_bytes = test_samples[idx]->num_bytes;

            size_t in_off = 0;
            size_t out_off = 0;

            const size_t actual_samples =
                mapper.map(test_samples[idx]->samples, in_bytes, in_off, encoded_samples,
                           out_bytes, out_off, test_samples[idx]->num_samples);

            UNSIGNED_LONGS_EQUAL(test_samples[idx]->num_samples, actual_samples);

            UNSIGNED_LONGS_EQUAL(in_bytes * 8, in_off);
            UNSIGNED_LONGS_EQUAL(out_bytes * 8, out_off);
        }

        { // decode
            roc_log(LogDebug, "mapping %s to raw samples", test_samples[idx]->name);

            PcmSubformat in_fmt = test_samples[idx]->format;
            PcmSubformat out_fmt = PcmSubformat_Raw;

            PcmMapper mapper(in_fmt, out_fmt);

            UNSIGNED_LONGS_EQUAL(test_samples[idx]->num_samples,
                                 mapper.input_sample_count(test_samples[idx]->num_bytes));

            UNSIGNED_LONGS_EQUAL(test_samples[idx]->num_bytes,
                                 mapper.input_byte_count(test_samples[idx]->num_samples));

            const size_t in_bytes = test_samples[idx]->num_bytes;
            const size_t out_bytes = test_samples[idx]->num_samples * sizeof(sample_t);

            size_t in_off = 0;
            size_t out_off = 0;

            const size_t actual_samples =
                mapper.map(encoded_samples, in_bytes, in_off, decoded_samples, out_bytes,
                           out_off, test_samples[idx]->num_samples);

            UNSIGNED_LONGS_EQUAL(test_samples[idx]->num_samples, actual_samples);

            UNSIGNED_LONGS_EQUAL(in_bytes * 8, in_off);
            UNSIGNED_LONGS_EQUAL(out_bytes * 8, out_off);
        }

        { // compare
            roc_log(LogDebug, "comparing samples");

            for (size_t n = 0; n < test_samples[idx]->num_samples; n++) {
                DOUBLES_EQUAL(test_samples[idx]->samples[n], decoded_samples[n], Epsilon);
            }
        }
    }
}

} // namespace audio
} // namespace roc
