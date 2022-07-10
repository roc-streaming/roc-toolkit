/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/channel_mapper.h"

namespace roc {
namespace audio {

namespace {

enum { MaxSamples = 100 };

const double Epsilon = 0.000001;

void check(sample_t* input,
           sample_t* output,
           size_t n_samples,
           packet::channel_mask_t in_chans,
           packet::channel_mask_t out_chans) {
    sample_t actual_output[MaxSamples] = {};

    Frame in_frame(input, n_samples * packet::num_channels(in_chans));
    Frame out_frame(actual_output, n_samples * packet::num_channels(out_chans));

    ChannelMapper mapper(in_chans, out_chans);
    mapper.map(in_frame, out_frame);

    for (size_t n = 0; n < n_samples; n++) {
        DOUBLES_EQUAL(output[n], actual_output[n], Epsilon);
    }
}

} // namespace

TEST_GROUP(channel_mapper) {};

TEST(channel_mapper, mask_equal) {
    enum { NumSamples = 5, InChans = 0x3, OutChans = 0x3 };

    sample_t input[NumSamples * 2] = {
        0.1f, 0.2f, //
        0.3f, 0.4f, //
        0.5f, 0.6f, //
        0.7f, 0.8f, //
        0.9f, 1.0f, //
    };

    sample_t output[NumSamples * 2] = {
        0.1f, 0.2f, //
        0.3f, 0.4f, //
        0.5f, 0.6f, //
        0.7f, 0.8f, //
        0.9f, 1.0f, //
    };

    check(input, output, NumSamples, InChans, OutChans);
}

TEST(channel_mapper, mask_subset) {
    enum { NumSamples = 5, InChans = 0x2, OutChans = 0x3 };

    sample_t input[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    sample_t output[NumSamples * 2] = {
        0.0f, 0.1f, //
        0.0f, 0.2f, //
        0.0f, 0.3f, //
        0.0f, 0.4f, //
        0.0f, 0.5f, //
    };

    check(input, output, NumSamples, InChans, OutChans);
}

TEST(channel_mapper, mask_superset) {
    enum { NumSamples = 5, InChans = 0x7, OutChans = 0x3 };

    sample_t input[NumSamples * 3] = {
        -0.1f, 0.1f, 0.8f, //
        -0.2f, 0.2f, 0.8f, //
        -0.3f, 0.3f, 0.8f, //
        -0.4f, 0.4f, 0.8f, //
        -0.5f, 0.5f, 0.8f, //
    };

    sample_t output[NumSamples * 2] = {
        -0.1f, 0.1f, //
        -0.2f, 0.2f, //
        -0.3f, 0.3f, //
        -0.4f, 0.4f, //
        -0.5f, 0.5f, //
    };

    check(input, output, NumSamples, InChans, OutChans);
}

TEST(channel_mapper, mask_overlap) {
    enum { NumSamples = 5, InChans = 0x5, OutChans = 0x3 };

    sample_t input[NumSamples * 3] = {
        -0.1f, 0.8f, //
        -0.2f, 0.8f, //
        -0.3f, 0.8f, //
        -0.4f, 0.8f, //
        -0.5f, 0.8f, //
    };

    sample_t output[NumSamples * 2] = {
        -0.1f, 0.0f, //
        -0.2f, 0.0f, //
        -0.3f, 0.0f, //
        -0.4f, 0.0f, //
        -0.5f, 0.0f, //
    };

    check(input, output, NumSamples, InChans, OutChans);
}

} // namespace audio
} // namespace roc
