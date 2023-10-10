/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/channel_mapper.h"
#include "roc_core/macro_helpers.h"

namespace roc {
namespace audio {

namespace {

enum { MaxSamples = 100 };

const double Epsilon = 0.000001;

void check(sample_t* input,
           sample_t* output,
           size_t n_samples,
           ChannelLayout in_layout,
           ChannelOrder in_order,
           ChannelMask in_mask,
           ChannelLayout out_layout,
           ChannelOrder out_order,
           ChannelMask out_mask) {
    ChannelSet in_chans;
    in_chans.set_layout(in_layout);
    in_chans.set_order(in_order);
    in_chans.set_channel_mask(in_mask);

    ChannelSet out_chans;
    out_chans.set_layout(out_layout);
    out_chans.set_order(out_order);
    out_chans.set_channel_mask(out_mask);

    sample_t actual_output[MaxSamples] = {};

    ChannelMapper mapper(in_chans, out_chans);
    mapper.map(input, n_samples * in_chans.num_channels(), actual_output,
               n_samples * out_chans.num_channels());

    for (size_t n = 0; n < n_samples; n++) {
        DOUBLES_EQUAL(output[n], actual_output[n], Epsilon);
    }
}

} // namespace

TEST_GROUP(channel_mapper) {};

TEST(channel_mapper, mono_mono) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_Mono,
        OutChans = ChanMask_Surround_Mono
    };

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

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

TEST(channel_mapper, mono_stereo) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_Mono,
        OutChans = ChanMask_Surround_Stereo
    };

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

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

TEST(channel_mapper, stereo_mono) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_Stereo,
        OutChans = ChanMask_Surround_Mono
    };

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

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

TEST(channel_mapper, stereo_stereo) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_Stereo,
        OutChans = ChanMask_Surround_Stereo
    };

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

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

TEST(channel_mapper, surround_61_41) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_6_1,
        OutChans = ChanMask_Surround_4_1
    };

    sample_t clev = 1.000f / (1.000f + 0.707f);
    sample_t slev = 0.707f / (1.000f + 0.707f);

    sample_t input[NumSamples * 7] = {
        // FL     FC     FR     BL     BC     BR    LFE
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, 0.26f, 0.27f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, 0.36f, 0.37f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, 0.46f, 0.47f, // 4
    };

    sample_t output[NumSamples * 5] = {
        // 0
        clev * 0.01f + slev * 0.02f, // FL
        clev * 0.03f + slev * 0.02f, // FR
        clev * 0.04f + slev * 0.05f, // BL
        clev * 0.06f + slev * 0.05f, // BR
        0.07f,                       // LFE
        // 1
        clev * 0.11f + slev * 0.12f, // FL
        clev * 0.13f + slev * 0.12f, // FR
        clev * 0.14f + slev * 0.15f, // BL
        clev * 0.16f + slev * 0.15f, // BR
        0.17f,                       // LFE
        // 2
        clev * 0.21f + slev * 0.22f, // FL
        clev * 0.23f + slev * 0.22f, // FR
        clev * 0.24f + slev * 0.25f, // BL
        clev * 0.26f + slev * 0.25f, // BR
        0.27f,                       // LFE
        // 3
        clev * 0.31f + slev * 0.32f, // FL
        clev * 0.33f + slev * 0.32f, // FR
        clev * 0.34f + slev * 0.35f, // BL
        clev * 0.36f + slev * 0.35f, // BR
        0.37f,                       // LFE
        // 4
        clev * 0.41f + slev * 0.42f, // FL
        clev * 0.43f + slev * 0.42f, // FR
        clev * 0.44f + slev * 0.45f, // BL
        clev * 0.46f + slev * 0.45f, // BR
        0.47f,                       // LFE
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

TEST(channel_mapper, surround_60_41) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_6_0,
        OutChans = ChanMask_Surround_4_1
    };

    sample_t clev = 1.000f / (1.000f + 0.707f);
    sample_t slev = 0.707f / (1.000f + 0.707f);

    sample_t input[NumSamples * 7] = {
        // FL     FC     FR     BL     BC     BR
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, 0.26f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, 0.36f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, 0.46f, // 4
    };

    sample_t output[NumSamples * 5] = {
        // 0
        clev * 0.01f + slev * 0.02f, // FL
        clev * 0.03f + slev * 0.02f, // FR
        clev * 0.04f + slev * 0.05f, // BL
        clev * 0.06f + slev * 0.05f, // BR
        0.f,                         // LFE
        // 1
        clev * 0.11f + slev * 0.12f, // FL
        clev * 0.13f + slev * 0.12f, // FR
        clev * 0.14f + slev * 0.15f, // BL
        clev * 0.16f + slev * 0.15f, // BR
        0.f,                         // LFE
        // 2
        clev * 0.21f + slev * 0.22f, // FL
        clev * 0.23f + slev * 0.22f, // FR
        clev * 0.24f + slev * 0.25f, // BL
        clev * 0.26f + slev * 0.25f, // BR
        0.f,                         // LFE
        // 3
        clev * 0.31f + slev * 0.32f, // FL
        clev * 0.33f + slev * 0.32f, // FR
        clev * 0.34f + slev * 0.35f, // BL
        clev * 0.36f + slev * 0.35f, // BR
        0.f,                         // LFE
        // 4
        clev * 0.41f + slev * 0.42f, // FL
        clev * 0.43f + slev * 0.42f, // FR
        clev * 0.44f + slev * 0.45f, // BL
        clev * 0.46f + slev * 0.45f, // BR
        0.f,                         // LFE
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

TEST(channel_mapper, surround_61_40) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_6_1,
        OutChans = ChanMask_Surround_4_0
    };

    sample_t clev = 1.000f / (1.000f + 0.707f);
    sample_t slev = 0.707f / (1.000f + 0.707f);

    sample_t input[NumSamples * 7] = {
        // FL     FC     FR     BL     BC     BR    LFE
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, 0.26f, 0.27f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, 0.36f, 0.37f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, 0.46f, 0.47f, // 4
    };

    sample_t output[NumSamples * 5] = {
        // 0
        clev * 0.01f + slev * 0.02f, // FL
        clev * 0.03f + slev * 0.02f, // FR
        clev * 0.04f + slev * 0.05f, // BL
        clev * 0.06f + slev * 0.05f, // BR
        // 1
        clev * 0.11f + slev * 0.12f, // FL
        clev * 0.13f + slev * 0.12f, // FR
        clev * 0.14f + slev * 0.15f, // BL
        clev * 0.16f + slev * 0.15f, // BR
        // 2
        clev * 0.21f + slev * 0.22f, // FL
        clev * 0.23f + slev * 0.22f, // FR
        clev * 0.24f + slev * 0.25f, // BL
        clev * 0.26f + slev * 0.25f, // BR
        // 3
        clev * 0.31f + slev * 0.32f, // FL
        clev * 0.33f + slev * 0.32f, // FR
        clev * 0.34f + slev * 0.35f, // BL
        clev * 0.36f + slev * 0.35f, // BR
        // 4
        clev * 0.41f + slev * 0.42f, // FL
        clev * 0.43f + slev * 0.42f, // FR
        clev * 0.44f + slev * 0.45f, // BL
        clev * 0.46f + slev * 0.45f, // BR
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

TEST(channel_mapper, surround_6x_4x) {
    enum {
        NumSamples = 5,
        // missing FC
        InChans = (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontRight)
            | (1 << ChanPos_BackLeft) | (1 << ChanPos_BackCenter)
            | (1 << ChanPos_BackRight),
        // missing FR
        OutChans =
            (1 << ChanPos_FrontLeft) | (1 << ChanPos_BackLeft) | (1 << ChanPos_BackRight)
    };

    sample_t clev = 1.000f / (1.000f + 0.707f);
    sample_t slev = 0.707f / (1.000f + 0.707f);

    sample_t input[NumSamples * 5] = {
        // FL     FR     BL     BC     BR
        0.01f, 0.03f, 0.04f, 0.05f, 0.06f, // 0
        0.11f, 0.13f, 0.14f, 0.15f, 0.16f, // 1
        0.21f, 0.23f, 0.24f, 0.25f, 0.26f, // 2
        0.31f, 0.33f, 0.34f, 0.35f, 0.36f, // 3
        0.41f, 0.43f, 0.44f, 0.45f, 0.46f, // 4
    };

    sample_t output[NumSamples * 3] = {
        // 0
        0.01f,                       // FL
        clev * 0.04f + slev * 0.05f, // BL
        clev * 0.06f + slev * 0.05f, // BR
        // 1
        0.11f,                       // FL
        clev * 0.14f + slev * 0.15f, // BL
        clev * 0.16f + slev * 0.15f, // BR
        // 2
        0.21f,                       // FL
        clev * 0.24f + slev * 0.25f, // BL
        clev * 0.26f + slev * 0.25f, // BR
        // 3
        0.31f,                       // FL
        clev * 0.34f + slev * 0.35f, // BL
        clev * 0.36f + slev * 0.35f, // BR
        // 4
        0.41f,                       // FL
        clev * 0.44f + slev * 0.45f, // BL
        clev * 0.46f + slev * 0.45f, // BR
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

TEST(channel_mapper, surround_41_61) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_4_1,
        OutChans = ChanMask_Surround_6_1
    };

    sample_t lev = (1.f / 0.707f) / (2.f / 0.707f);

    sample_t input[NumSamples * 5] = {
        // FL     FR     BL     BR    LFE
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, // 4
    };

    sample_t output[NumSamples * 7] = {
        // 0
        0.01f,                     // FL
        lev * 0.01f + lev * 0.02f, // FC
        0.02f,                     // FR
        0.03f,                     // BL
        lev * 0.03f + lev * 0.04f, // BC
        0.04f,                     // BR
        0.05f,                     // LFE
        // 1
        0.11f,                     // FL
        lev * 0.11f + lev * 0.12f, // FC
        0.12f,                     // FR
        0.13f,                     // BL
        lev * 0.13f + lev * 0.14f, // BC
        0.14f,                     // BR
        0.15f,                     // LFE
        // 2
        0.21f,                     // FL
        lev * 0.21f + lev * 0.22f, // FC
        0.22f,                     // FR
        0.23f,                     // BL
        lev * 0.23f + lev * 0.24f, // BC
        0.24f,                     // BR
        0.25f,                     // LFE
        // 3
        0.31f,                     // FL
        lev * 0.31f + lev * 0.32f, // FC
        0.32f,                     // FR
        0.33f,                     // BL
        lev * 0.33f + lev * 0.34f, // BC
        0.34f,                     // BR
        0.35f,                     // LFE
        // 4
        0.41f,                     // FL
        lev * 0.41f + lev * 0.42f, // FC
        0.42f,                     // FR
        0.43f,                     // BL
        lev * 0.43f + lev * 0.44f, // BC
        0.44f,                     // BR
        0.45f,                     // LFE
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

TEST(channel_mapper, surround_1ch) {
    enum {
        NumSamples = 5,
    };

    const ChannelMask masks[] = {
        ChanMask_Surround_Mono,   //
        ChanMask_Surround_Stereo, //
        ChanMask_Surround_2_1,    //
        ChanMask_Surround_3_0,    //
        ChanMask_Surround_3_1,    //
        ChanMask_Surround_4_0,    //
        ChanMask_Surround_4_1,    //
        ChanMask_Surround_5_0,    //
        ChanMask_Surround_5_1,    //
        ChanMask_Surround_5_1_2,  //
        ChanMask_Surround_5_1_4,  //
        ChanMask_Surround_6_0,    //
        ChanMask_Surround_6_1,    //
        ChanMask_Surround_7_0,    //
        ChanMask_Surround_7_1,    //
        ChanMask_Surround_7_1_2,  //
        ChanMask_Surround_7_1_4,  //
    };

    for (size_t i = 0; i < ROC_ARRAY_SIZE(masks); i++) {
        for (size_t j = 0; j < ROC_ARRAY_SIZE(masks); j++) {
            ChannelSet in_chans;
            in_chans.set_layout(ChanLayout_Surround);
            in_chans.set_order(ChanOrder_Smpte);
            in_chans.set_channel_mask(masks[i]);

            ChannelSet out_chans;
            out_chans.set_layout(ChanLayout_Surround);
            out_chans.set_order(ChanOrder_Smpte);
            out_chans.set_channel_mask(masks[j]);

            for (size_t ch = 0; ch < ChanPos_Max; ch++) {
                if (in_chans.has_channel(ch) && out_chans.has_channel(ch)) {
                    size_t in_off = 0;
                    for (size_t in_ch = in_chans.first_channel();
                         in_ch <= in_chans.last_channel(); in_ch++) {
                        if (in_ch == ch) {
                            break;
                        }
                        if (in_chans.has_channel(in_ch)) {
                            in_off++;
                        }
                    }

                    size_t out_off = 0;
                    for (size_t out_ch = out_chans.first_channel();
                         out_ch <= out_chans.last_channel(); out_ch++) {
                        if (out_ch == ch) {
                            break;
                        }
                        if (out_chans.has_channel(out_ch)) {
                            out_off++;
                        }
                    }

                    sample_t in_buf[NumSamples * ChanPos_Max] = {};
                    sample_t out_buf[NumSamples * ChanPos_Max] = {};

                    for (size_t ns = 0; ns < NumSamples; ns++) {
                        in_buf[ns * in_chans.num_channels() + in_off] = 0.12345f;
                    }

                    ChannelMapper mapper(in_chans, out_chans);
                    mapper.map(in_buf, NumSamples * in_chans.num_channels(), out_buf,
                               NumSamples * out_chans.num_channels());

                    for (size_t ns = 0; ns < NumSamples; ns++) {
                        CHECK(out_buf[ns * out_chans.num_channels() + out_off] > 0.f);
                    }
                }
            }
        }
    }
}

TEST(channel_mapper, mono_multitrack) {
    enum { NumSamples = 5, InChans = ChanMask_Surround_Mono, OutChans = 0x88 };

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

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Multitrack, ChanOrder_None, OutChans);
}

TEST(channel_mapper, stereo_multitrack) {
    enum { NumSamples = 5, InChans = ChanMask_Surround_Stereo, OutChans = 0x888 };

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

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Multitrack, ChanOrder_None, OutChans);
}

TEST(channel_mapper, multitrack_mono) {
    enum { NumSamples = 5, InChans = 0x88, OutChans = ChanMask_Surround_Mono };

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

    check(input, output, NumSamples, ChanLayout_Multitrack, ChanOrder_None, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

TEST(channel_mapper, multitrack_stereo) {
    enum { NumSamples = 5, InChans = 0x888, OutChans = ChanMask_Surround_Stereo };

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

    check(input, output, NumSamples, ChanLayout_Multitrack, ChanOrder_None, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
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

    check(input, output, NumSamples, ChanLayout_Multitrack, ChanOrder_None, InChans,
          ChanLayout_Multitrack, ChanOrder_None, OutChans);
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

    check(input, output, NumSamples, ChanLayout_Multitrack, ChanOrder_None, InChans,
          ChanLayout_Multitrack, ChanOrder_None, OutChans);
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

    check(input, output, NumSamples, ChanLayout_Multitrack, ChanOrder_None, InChans,
          ChanLayout_Multitrack, ChanOrder_None, OutChans);
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

    check(input, output, NumSamples, ChanLayout_Multitrack, ChanOrder_None, InChans,
          ChanLayout_Multitrack, ChanOrder_None, OutChans);
}

} // namespace audio
} // namespace roc
