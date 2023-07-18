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
           ChannelLayout in_layout,
           ChannelMask in_mask,
           ChannelLayout out_layout,
           ChannelMask out_mask) {
    ChannelSet in_chans;
    in_chans.set_layout(in_layout);
    in_chans.set_channel_mask(in_mask);

    ChannelSet out_chans;
    out_chans.set_layout(out_layout);
    out_chans.set_channel_mask(out_mask);

    sample_t actual_output[MaxSamples] = {};

    Frame in_frame(input, n_samples * in_chans.num_channels());
    Frame out_frame(actual_output, n_samples * out_chans.num_channels());

    ChannelMapper mapper(in_chans, out_chans);
    mapper.map(in_frame, out_frame);

    for (size_t n = 0; n < n_samples; n++) {
        DOUBLES_EQUAL(output[n], actual_output[n], Epsilon);
    }
}

} // namespace

TEST_GROUP(channel_mapper) {};

TEST(channel_mapper, mono_mono) {
    enum { NumSamples = 5, InChans = 0x1, OutChans = 0x1 };

    sample_t input[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    sample_t output[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    check(input, output, NumSamples, ChannelLayout_Mono, InChans, ChannelLayout_Mono,
          OutChans);
}

TEST(channel_mapper, mono_stereo) {
    enum { NumSamples = 5, InChans = 0x1, OutChans = 0x3 };

    sample_t input[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    sample_t output[NumSamples * 2] = {
        0.1f, 0.1f, //
        0.2f, 0.2f, //
        0.3f, 0.3f, //
        0.4f, 0.4f, //
        0.5f, 0.5f, //
    };

    check(input, output, NumSamples, ChannelLayout_Mono, InChans, ChannelLayout_Surround,
          OutChans);
}

TEST(channel_mapper, stereo_mono_dup) {
    enum { NumSamples = 5, InChans = 0x3, OutChans = 0x1 };

    sample_t input[NumSamples * 2] = {
        0.1f, 0.1f, //
        0.2f, 0.2f, //
        0.3f, 0.3f, //
        0.4f, 0.4f, //
        0.5f, 0.5f, //
    };

    sample_t output[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    check(input, output, NumSamples, ChannelLayout_Surround, InChans, ChannelLayout_Mono,
          OutChans);
}

TEST(channel_mapper, stereo_mono_avg) {
    enum { NumSamples = 5, InChans = 0x3, OutChans = 0x1 };

    sample_t input[NumSamples * 2] = {
        0.1f, 0.3f, //
        0.2f, 0.4f, //
        0.3f, 0.5f, //
        0.4f, 0.6f, //
        0.5f, 0.7f, //
    };

    sample_t output[NumSamples] = {
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
        0.6f, //
    };

    check(input, output, NumSamples, ChannelLayout_Surround, InChans, ChannelLayout_Mono,
          OutChans);
}

TEST(channel_mapper, stereo_stereo) {
    enum { NumSamples = 5, InChans = 0x3, OutChans = 0x3 };

    sample_t input[NumSamples * 2] = {
        0.1f, 0.3f, //
        0.2f, 0.4f, //
        0.3f, 0.5f, //
        0.4f, 0.6f, //
        0.5f, 0.7f, //
    };

    sample_t output[NumSamples * 2] = {
        0.1f, 0.3f, //
        0.2f, 0.4f, //
        0.3f, 0.5f, //
        0.4f, 0.6f, //
        0.5f, 0.7f, //
    };

    check(input, output, NumSamples, ChannelLayout_Surround, InChans,
          ChannelLayout_Surround, OutChans);
}

TEST(channel_mapper, mono_multitrack) {
    enum { NumSamples = 5, InChans = 0x1, OutChans = 0x88 };

    sample_t input[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    sample_t output[NumSamples * 2] = {
        0.1f, 0.0f, //
        0.2f, 0.0f, //
        0.3f, 0.0f, //
        0.4f, 0.0f, //
        0.5f, 0.0f, //
    };

    check(input, output, NumSamples, ChannelLayout_Mono, InChans,
          ChannelLayout_Multitrack, OutChans);
}

TEST(channel_mapper, stereo_multitrack) {
    enum { NumSamples = 5, InChans = 0x3, OutChans = 0x888 };

    sample_t input[NumSamples * 2] = {
        0.1f, -0.1f, //
        0.2f, -0.2f, //
        0.3f, -0.3f, //
        0.4f, -0.4f, //
        0.5f, -0.5f, //
    };

    sample_t output[NumSamples * 3] = {
        0.1f, -0.1f, 0.0f, //
        0.2f, -0.2f, 0.0f, //
        0.3f, -0.3f, 0.0f, //
        0.4f, -0.4f, 0.0f, //
        0.5f, -0.5f, 0.0f, //
    };

    check(input, output, NumSamples, ChannelLayout_Surround, InChans,
          ChannelLayout_Multitrack, OutChans);
}

TEST(channel_mapper, multitrack_mono) {
    enum { NumSamples = 5, InChans = 0x88, OutChans = 0x1 };

    sample_t input[NumSamples * 2] = {
        0.1f, -0.1f, //
        0.2f, -0.2f, //
        0.3f, -0.3f, //
        0.4f, -0.4f, //
        0.5f, -0.5f, //
    };

    sample_t output[NumSamples] = {
        0.1f, //
        0.2f, //
        0.3f, //
        0.4f, //
        0.5f, //
    };

    check(input, output, NumSamples, ChannelLayout_Multitrack, InChans,
          ChannelLayout_Mono, OutChans);
}

TEST(channel_mapper, multitrack_stereo) {
    enum { NumSamples = 5, InChans = 0x888, OutChans = 0x3 };

    sample_t input[NumSamples * 3] = {
        0.1f, -0.1f, 0.33f, //
        0.2f, -0.2f, 0.33f, //
        0.3f, -0.3f, 0.33f, //
        0.4f, -0.4f, 0.33f, //
        0.5f, -0.5f, 0.33f, //
    };

    sample_t output[NumSamples * 2] = {
        0.1f, -0.1f, //
        0.2f, -0.2f, //
        0.3f, -0.3f, //
        0.4f, -0.4f, //
        0.5f, -0.5f, //
    };

    check(input, output, NumSamples, ChannelLayout_Multitrack, InChans,
          ChannelLayout_Surround, OutChans);
}

TEST(channel_mapper, multitrack_same) {
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

    check(input, output, NumSamples, ChannelLayout_Multitrack, InChans,
          ChannelLayout_Multitrack, OutChans);
}

TEST(channel_mapper, multitrack_subset) {
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

    check(input, output, NumSamples, ChannelLayout_Multitrack, InChans,
          ChannelLayout_Multitrack, OutChans);
}

TEST(channel_mapper, multitrack_superset) {
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

    check(input, output, NumSamples, ChannelLayout_Multitrack, InChans,
          ChannelLayout_Multitrack, OutChans);
}

TEST(channel_mapper, multitrack_overlap) {
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

    check(input, output, NumSamples, ChannelLayout_Multitrack, InChans,
          ChannelLayout_Multitrack, OutChans);
}

} // namespace audio
} // namespace roc
