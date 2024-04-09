/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/channel_mapper.h"
#include "roc_audio/channel_tables.h"
#include "roc_core/macro_helpers.h"

namespace roc {
namespace audio {

namespace {

enum { MaxSamples = 100 };

const double Epsilon = 0.000001;

void check(const sample_t* input,
           const sample_t* output,
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
    in_chans.set_mask(in_mask);

    ChannelSet out_chans;
    out_chans.set_layout(out_layout);
    out_chans.set_order(out_order);
    out_chans.set_mask(out_mask);

    sample_t actual_output[MaxSamples] = {};
    memset(actual_output, 0xff, MaxSamples * sizeof(sample_t));

    ChannelMapper mapper(in_chans, out_chans);
    mapper.map(input, n_samples * in_chans.num_channels(), actual_output,
               n_samples * out_chans.num_channels());

    for (size_t n = 0; n < n_samples * out_chans.num_channels(); n++) {
        DOUBLES_EQUAL(output[n], actual_output[n], Epsilon);
    }
}

} // namespace

TEST_GROUP(channel_mapper) {};

// verbatim copy
TEST(channel_mapper, mono_to_mono) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_Mono,
        OutChans = ChanMask_Surround_Mono
    };

    const sample_t input[NumSamples] = {
        // FC
        0.01f, // 0
        0.02f, // 1
        0.03f, // 2
        0.04f, // 3
        0.05f, // 4
    };

    const sample_t output[NumSamples] = {
        // FC
        0.01f, // 0
        0.02f, // 1
        0.03f, // 2
        0.04f, // 3
        0.05f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// upmixing
TEST(channel_mapper, mono_to_stereo) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_Mono,
        OutChans = ChanMask_Surround_Stereo
    };

    const sample_t input[NumSamples] = {
        // FC
        0.01f, // 0
        0.02f, // 1
        0.03f, // 2
        0.04f, // 3
        0.05f, // 4
    };

    const sample_t output[NumSamples * 2] = {
        // FL     FR
        0.01f, 0.01f, // 0
        0.02f, 0.02f, // 1
        0.03f, 0.03f, // 2
        0.04f, 0.04f, // 3
        0.05f, 0.05f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// downmixing
TEST(channel_mapper, stereo_to_mono) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_Stereo,
        OutChans = ChanMask_Surround_Mono
    };

    const sample_t input[NumSamples * 2] = {
        // FL     FR
        0.01f, 0.03f, // 0
        0.02f, 0.04f, // 1
        0.03f, 0.05f, // 2
        0.04f, 0.06f, // 3
        0.05f, 0.07f, // 4
    };

    const sample_t output[NumSamples] = {
        // FC
        0.02f, // 0
        0.03f, // 1
        0.04f, // 2
        0.05f, // 3
        0.06f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// verbatim copy
TEST(channel_mapper, stereo_to_stereo) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_Stereo,
        OutChans = ChanMask_Surround_Stereo
    };

    const sample_t input[NumSamples * 2] = {
        // FL     FR
        0.01f, 0.03f, // 0
        0.02f, 0.04f, // 1
        0.03f, 0.05f, // 2
        0.04f, 0.06f, // 3
        0.05f, 0.07f, // 4
    };

    const sample_t output[NumSamples * 2] = {
        // FL     FR
        0.01f, 0.03f, // 0
        0.02f, 0.04f, // 1
        0.03f, 0.05f, // 2
        0.04f, 0.06f, // 3
        0.05f, 0.07f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// downmixing
TEST(channel_mapper, surround_61_to_41) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_6_1,
        OutChans = ChanMask_Surround_4_1
    };

    const sample_t clev = 1.000f / (1.000f + 0.707f);
    const sample_t slev = 0.707f / (1.000f + 0.707f);

    const sample_t input[NumSamples * 7] = {
        // FL     FR     FC    LFE     BL     BR     BC
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, 0.26f, 0.27f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, 0.36f, 0.37f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, 0.46f, 0.47f, // 4
    };

    const sample_t output[NumSamples * 5] = {
        // 0
        clev * 0.01f + slev * 0.03f, // FL
        clev * 0.02f + slev * 0.03f, // FR
        0.04f,                       // LFE
        clev * 0.05f + slev * 0.07f, // BL
        clev * 0.06f + slev * 0.07f, // BR
        // 1
        clev * 0.11f + slev * 0.13f, // FL
        clev * 0.12f + slev * 0.13f, // FR
        0.14f,                       // LFE
        clev * 0.15f + slev * 0.17f, // BL
        clev * 0.16f + slev * 0.17f, // BR
        // 2
        clev * 0.21f + slev * 0.23f, // FL
        clev * 0.22f + slev * 0.23f, // FR
        0.24f,                       // LFE
        clev * 0.25f + slev * 0.27f, // BL
        clev * 0.26f + slev * 0.27f, // BR
        // 3
        clev * 0.31f + slev * 0.33f, // FL
        clev * 0.32f + slev * 0.33f, // FR
        0.34f,                       // LFE
        clev * 0.35f + slev * 0.37f, // BL
        clev * 0.36f + slev * 0.37f, // BR
        // 4
        clev * 0.41f + slev * 0.43f, // FL
        clev * 0.42f + slev * 0.43f, // FR
        0.44f,                       // LFE
        clev * 0.45f + slev * 0.47f, // BL
        clev * 0.46f + slev * 0.47f, // BR
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// downmixing + adding zero LFE
TEST(channel_mapper, surround_60_to_41) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_6_0,
        OutChans = ChanMask_Surround_4_1
    };

    const sample_t clev = 1.000f / (1.000f + 0.707f);
    const sample_t slev = 0.707f / (1.000f + 0.707f);

    const sample_t input[NumSamples * 7] = {
        // FL     FR     FC     BL     BR     BC
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, 0.26f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, 0.36f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, 0.46f, // 4
    };

    const sample_t output[NumSamples * 5] = {
        // 0
        clev * 0.01f + slev * 0.03f, // FL
        clev * 0.02f + slev * 0.03f, // FR
        0.f,                         // LFE
        clev * 0.04f + slev * 0.06f, // BL
        clev * 0.05f + slev * 0.06f, // BR
        // 1
        clev * 0.11f + slev * 0.13f, // FL
        clev * 0.12f + slev * 0.13f, // FR
        0.f,                         // LFE
        clev * 0.14f + slev * 0.16f, // BL
        clev * 0.15f + slev * 0.16f, // BR
        // 2
        clev * 0.21f + slev * 0.23f, // FL
        clev * 0.22f + slev * 0.23f, // FR
        0.f,                         // LFE
        clev * 0.24f + slev * 0.26f, // BL
        clev * 0.25f + slev * 0.26f, // BR
        // 3
        clev * 0.31f + slev * 0.33f, // FL
        clev * 0.32f + slev * 0.33f, // FR
        0.f,                         // LFE
        clev * 0.34f + slev * 0.36f, // BL
        clev * 0.35f + slev * 0.36f, // BR
        // 4
        clev * 0.41f + slev * 0.43f, // FL
        clev * 0.42f + slev * 0.43f, // FR
        0.f,                         // LFE
        clev * 0.44f + slev * 0.46f, // BL
        clev * 0.45f + slev * 0.46f, // BR
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// downmixing + removing LFE
TEST(channel_mapper, surround_61_to_40) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_6_1,
        OutChans = ChanMask_Surround_4_0
    };

    const sample_t clev = 1.000f / (1.000f + 0.707f);
    const sample_t slev = 0.707f / (1.000f + 0.707f);

    const sample_t input[NumSamples * 7] = {
        // FL     FR     FC    LFE     BL     BR     BC
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, 0.26f, 0.27f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, 0.36f, 0.37f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, 0.46f, 0.47f, // 4
    };

    const sample_t output[NumSamples * 5] = {
        // 0
        clev * 0.01f + slev * 0.03f, // FL
        clev * 0.02f + slev * 0.03f, // FR
        clev * 0.05f + slev * 0.07f, // BL
        clev * 0.06f + slev * 0.07f, // BR
        // 1
        clev * 0.11f + slev * 0.13f, // FL
        clev * 0.12f + slev * 0.13f, // FR
        clev * 0.15f + slev * 0.17f, // BL
        clev * 0.16f + slev * 0.17f, // BR
        // 2
        clev * 0.21f + slev * 0.23f, // FL
        clev * 0.22f + slev * 0.23f, // FR
        clev * 0.25f + slev * 0.27f, // BL
        clev * 0.26f + slev * 0.27f, // BR
        // 3
        clev * 0.31f + slev * 0.33f, // FL
        clev * 0.32f + slev * 0.33f, // FR
        clev * 0.35f + slev * 0.37f, // BL
        clev * 0.36f + slev * 0.37f, // BR
        // 4
        clev * 0.41f + slev * 0.43f, // FL
        clev * 0.42f + slev * 0.43f, // FR
        clev * 0.45f + slev * 0.47f, // BL
        clev * 0.46f + slev * 0.47f, // BR
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// handling of incomplete masks
TEST(channel_mapper, surround_6x_to_4x) {
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

    const sample_t clev = 1.000f / (1.000f + 0.707f);
    const sample_t slev = 0.707f / (1.000f + 0.707f);

    const sample_t input[NumSamples * 5] = {
        // FL     FR     BL     BR     BC
        0.01f, 0.03f, 0.04f, 0.05f, 0.06f, // 0
        0.11f, 0.13f, 0.14f, 0.15f, 0.16f, // 1
        0.21f, 0.23f, 0.24f, 0.25f, 0.26f, // 2
        0.31f, 0.33f, 0.34f, 0.35f, 0.36f, // 3
        0.41f, 0.43f, 0.44f, 0.45f, 0.46f, // 4
    };

    const sample_t output[NumSamples * 3] = {
        // 0
        0.01f,                       // FL
        clev * 0.04f + slev * 0.06f, // BL
        clev * 0.05f + slev * 0.06f, // BR
        // 1
        0.11f,                       // FL
        clev * 0.14f + slev * 0.16f, // BL
        clev * 0.15f + slev * 0.16f, // BR
        // 2
        0.21f,                       // FL
        clev * 0.24f + slev * 0.26f, // BL
        clev * 0.25f + slev * 0.26f, // BR
        // 3
        0.31f,                       // FL
        clev * 0.34f + slev * 0.36f, // BL
        clev * 0.35f + slev * 0.36f, // BR
        // 4
        0.41f,                       // FL
        clev * 0.44f + slev * 0.46f, // BL
        clev * 0.45f + slev * 0.46f, // BR
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// upmixing
TEST(channel_mapper, surround_41_to_61) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_4_1,
        OutChans = ChanMask_Surround_6_1
    };

    const sample_t lev = (1.f / 0.707f) / (2.f / 0.707f);

    const sample_t input[NumSamples * 5] = {
        // FL     FR    LFE     BL     BR
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, // 4
    };

    const sample_t output[NumSamples * 7] = {
        // 0
        0.01f,                     // FL
        0.02f,                     // FR
        lev * 0.01f + lev * 0.02f, // FC
        0.03f,                     // LFE
        0.04f,                     // BL
        0.05f,                     // BR
        lev * 0.04f + lev * 0.05f, // BC
        // 1
        0.11f,                     // FL
        0.12f,                     // FR
        lev * 0.11f + lev * 0.12f, // FC
        0.13f,                     // LFE
        0.14f,                     // BL
        0.15f,                     // BR
        lev * 0.14f + lev * 0.15f, // BC
        // 2
        0.21f,                     // FL
        0.22f,                     // FR
        lev * 0.21f + lev * 0.22f, // FC
        0.23f,                     // LFE
        0.24f,                     // BL
        0.25f,                     // BR
        lev * 0.24f + lev * 0.25f, // BC
        // 3
        0.31f,                     // FL
        0.32f,                     // FR
        lev * 0.31f + lev * 0.32f, // FC
        0.33f,                     // LFE
        0.34f,                     // BL
        0.35f,                     // BR
        lev * 0.34f + lev * 0.35f, // BC
        // 4
        0.41f,                     // FL
        0.42f,                     // FR
        lev * 0.41f + lev * 0.42f, // FC
        0.43f,                     // LFE
        0.44f,                     // BL
        0.45f,                     // BR
        lev * 0.44f + lev * 0.45f, // BC
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// input has one non-zero channel
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
            in_chans.set_mask(masks[i]);

            ChannelSet out_chans;
            out_chans.set_layout(ChanLayout_Surround);
            out_chans.set_order(ChanOrder_Smpte);
            out_chans.set_mask(masks[j]);

            for (size_t ch = 0; ch < ChanPos_Max; ch++) {
                if (in_chans.has_channel(ch) && out_chans.has_channel(ch)) {
                    size_t in_off = 0;
                    for (size_t order_off = 0;
                         ChanOrderTables[ChanOrder_Smpte].chans[order_off] != ChanPos_Max;
                         order_off++) {
                        const ChannelPosition in_ch =
                            ChanOrderTables[ChanOrder_Smpte].chans[order_off];
                        if (in_ch == (ChannelPosition)ch) {
                            break;
                        }
                        if (in_chans.has_channel(in_ch)) {
                            in_off++;
                        }
                    }

                    size_t out_off = 0;
                    for (size_t order_off = 0;
                         ChanOrderTables[ChanOrder_Smpte].chans[order_off] != ChanPos_Max;
                         order_off++) {
                        const ChannelPosition out_ch =
                            ChanOrderTables[ChanOrder_Smpte].chans[order_off];
                        if (out_ch == (ChannelPosition)ch) {
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

// reordering without remixing
TEST(channel_mapper, surround_61_smpte_to_61_alsa) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_6_1,
        OutChans = ChanMask_Surround_6_1
    };

    const sample_t input[NumSamples * 7] = {
        // FL     FR     FC    LFE     BL     BR     BC
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, 0.26f, 0.27f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, 0.36f, 0.37f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, 0.46f, 0.47f, // 4
    };

    const sample_t output[NumSamples * 7] = {
        // FL     FR     BL     BR     FC    LFE     BC
        0.01f, 0.02f, 0.05f, 0.06f, 0.03f, 0.04f, 0.07f, // 0
        0.11f, 0.12f, 0.15f, 0.16f, 0.13f, 0.14f, 0.17f, // 1
        0.21f, 0.22f, 0.25f, 0.26f, 0.23f, 0.24f, 0.27f, // 2
        0.31f, 0.32f, 0.35f, 0.36f, 0.33f, 0.34f, 0.37f, // 3
        0.41f, 0.42f, 0.45f, 0.46f, 0.43f, 0.44f, 0.47f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Alsa, OutChans);
}

// reordering without remixing
TEST(channel_mapper, surround_61_alsa_to_61_smpte) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_6_1,
        OutChans = ChanMask_Surround_6_1
    };

    const sample_t input[NumSamples * 7] = {
        // FL     FR     BL     BR     FC    LFE     BC
        0.01f, 0.02f, 0.05f, 0.06f, 0.03f, 0.04f, 0.07f, // 0
        0.11f, 0.12f, 0.15f, 0.16f, 0.13f, 0.14f, 0.17f, // 1
        0.21f, 0.22f, 0.25f, 0.26f, 0.23f, 0.24f, 0.27f, // 2
        0.31f, 0.32f, 0.35f, 0.36f, 0.33f, 0.34f, 0.37f, // 3
        0.41f, 0.42f, 0.45f, 0.46f, 0.43f, 0.44f, 0.47f, // 4
    };

    const sample_t output[NumSamples * 7] = {
        // FL     FR     FC    LFE     BL     BR     BC
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, 0.26f, 0.27f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, 0.36f, 0.37f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, 0.46f, 0.47f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Alsa, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// downmixing + reordering
TEST(channel_mapper, surround_61_smpte_to_41_alsa) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_6_1,
        OutChans = ChanMask_Surround_4_1
    };

    const sample_t clev = 1.000f / (1.000f + 0.707f);
    const sample_t slev = 0.707f / (1.000f + 0.707f);

    const sample_t input[NumSamples * 7] = {
        // FL     FR     FC    LFE     BL     BR     BC
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, 0.26f, 0.27f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, 0.36f, 0.37f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, 0.46f, 0.47f, // 4
    };

    const sample_t output[NumSamples * 5] = {
        // 0
        clev * 0.01f + slev * 0.03f, // FL
        clev * 0.02f + slev * 0.03f, // FR
        clev * 0.05f + slev * 0.07f, // BL
        clev * 0.06f + slev * 0.07f, // BR
        0.04f,                       // LFE
        // 1
        clev * 0.11f + slev * 0.13f, // FL
        clev * 0.12f + slev * 0.13f, // FR
        clev * 0.15f + slev * 0.17f, // BL
        clev * 0.16f + slev * 0.17f, // BR
        0.14f,                       // LFE
        // 2
        clev * 0.21f + slev * 0.23f, // FL
        clev * 0.22f + slev * 0.23f, // FR
        clev * 0.25f + slev * 0.27f, // BL
        clev * 0.26f + slev * 0.27f, // BR
        0.24f,                       // LFE
        // 3
        clev * 0.31f + slev * 0.33f, // FL
        clev * 0.32f + slev * 0.33f, // FR
        clev * 0.35f + slev * 0.37f, // BL
        clev * 0.36f + slev * 0.37f, // BR
        0.34f,                       // LFE
        // 4
        clev * 0.41f + slev * 0.43f, // FL
        clev * 0.42f + slev * 0.43f, // FR
        clev * 0.45f + slev * 0.47f, // BL
        clev * 0.46f + slev * 0.47f, // BR
        0.44f,                       // LFE
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Alsa, OutChans);
}

// upmixing + reordering
TEST(channel_mapper, surround_41_alsa_to_61_smpte) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_4_1,
        OutChans = ChanMask_Surround_6_1
    };

    const sample_t lev = (1.f / 0.707f) / (2.f / 0.707f);

    const sample_t input[NumSamples * 5] = {
        // FL     FR     BL     BR    LFE
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, // 4
    };

    const sample_t output[NumSamples * 7] = {
        // 0
        0.01f,                     // FL
        0.02f,                     // FR
        lev * 0.01f + lev * 0.02f, // FC
        0.05f,                     // LFE
        0.03f,                     // BL
        0.04f,                     // BR
        lev * 0.03f + lev * 0.04f, // BC
        // 1
        0.11f,                     // FL
        0.12f,                     // FR
        lev * 0.11f + lev * 0.12f, // FC
        0.15f,                     // LFE
        0.13f,                     // BL
        0.14f,                     // BR
        lev * 0.13f + lev * 0.14f, // BC
        // 2
        0.21f,                     // FL
        0.22f,                     // FR
        lev * 0.21f + lev * 0.22f, // FC
        0.25f,                     // LFE
        0.23f,                     // BL
        0.24f,                     // BR
        lev * 0.23f + lev * 0.24f, // BC
        // 3
        0.31f,                     // FL
        0.32f,                     // FR
        lev * 0.31f + lev * 0.32f, // FC
        0.35f,                     // LFE
        0.33f,                     // BL
        0.34f,                     // BR
        lev * 0.33f + lev * 0.34f, // BC
        // 4
        0.41f,                     // FL
        0.42f,                     // FR
        lev * 0.41f + lev * 0.42f, // FC
        0.45f,                     // LFE
        0.43f,                     // BL
        0.44f,                     // BR
        lev * 0.43f + lev * 0.44f, // BC
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Alsa, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// channels unsupported by output order are set to zero
TEST(channel_mapper, surround_512_smpte_to_512_alsa) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_5_1_2,
        OutChans = ChanMask_Surround_5_1_2
    };

    const sample_t clev = 1.000f / (1.000f + 0.707f);
    const sample_t slev = 0.707f / (1.000f + 0.707f);

    const sample_t input[NumSamples * 8] = {
        // FL     FR     FC    LFE     BL     BR    TML    TMR
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, 0.08f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.17f, 0.18f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, 0.26f, 0.27f, 0.28f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, 0.36f, 0.37f, 0.38f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, 0.46f, 0.47f, 0.48f, // 4
    };

    const sample_t output[NumSamples * 8] = {
        // 0
        clev * 0.01f + slev * 0.07f, // FL
        clev * 0.02f + slev * 0.08f, // FR
        clev * 0.05f + slev * 0.07f, // BL
        clev * 0.06f + slev * 0.08f, // BR
        0.03f,                       // FC
        0.04f,                       // LFE
        0.00f,                       // -
        0.00f,                       // -
        // 1
        clev * 0.11f + slev * 0.17f, // FL
        clev * 0.12f + slev * 0.18f, // FR
        clev * 0.15f + slev * 0.17f, // BL
        clev * 0.16f + slev * 0.18f, // BR
        0.13f,                       // FC
        0.14f,                       // LFE
        0.00f,                       // -
        0.00f,                       // -
        // 2
        clev * 0.21f + slev * 0.27f, // FL
        clev * 0.22f + slev * 0.28f, // FR
        clev * 0.25f + slev * 0.27f, // BL
        clev * 0.26f + slev * 0.28f, // BR
        0.23f,                       // FC
        0.24f,                       // LFE
        0.00f,                       // -
        0.00f,                       // -
        // 3
        clev * 0.31f + slev * 0.37f, // FL
        clev * 0.32f + slev * 0.38f, // FR
        clev * 0.35f + slev * 0.37f, // BL
        clev * 0.36f + slev * 0.38f, // BR
        0.33f,                       // FC
        0.34f,                       // LFE
        0.00f,                       // -
        0.00f,                       // -
        // 4
        clev * 0.41f + slev * 0.47f, // FL
        clev * 0.42f + slev * 0.48f, // FR
        clev * 0.45f + slev * 0.47f, // BL
        clev * 0.46f + slev * 0.48f, // BR
        0.43f,                       // FC
        0.44f,                       // LFE
        0.00f,                       // -
        0.00f,                       // -
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Surround, ChanOrder_Alsa, OutChans);
}

// channels unsupported by input order are ignored
TEST(channel_mapper, surround_512_alsa_to_512_smpte) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_5_1_2,
        OutChans = ChanMask_Surround_5_1_2
    };

    const sample_t lev = (1.f / 0.707f) / (2.f / 0.707f);

    const sample_t input[NumSamples * 8] = {
        // FL     FR     BL     BR     FC    LFE      -     -
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.99f, 0.99f, // 0
        0.11f, 0.12f, 0.13f, 0.14f, 0.15f, 0.16f, 0.99f, 0.99f, // 1
        0.21f, 0.22f, 0.23f, 0.24f, 0.25f, 0.26f, 0.99f, 0.99f, // 2
        0.31f, 0.32f, 0.33f, 0.34f, 0.35f, 0.36f, 0.99f, 0.99f, // 3
        0.41f, 0.42f, 0.43f, 0.44f, 0.45f, 0.46f, 0.99f, 0.99f, // 4
    };

    const sample_t output[NumSamples * 8] = {
        // 0
        0.01f,                     // FL
        0.02f,                     // FR
        0.05f,                     // FC
        0.06f,                     // LFE
        0.03f,                     // BL
        0.04f,                     // BR
        lev * 0.01f + lev * 0.03f, // TML
        lev * 0.02f + lev * 0.04f, // TMR
        // 1
        0.11f,                     // FL
        0.12f,                     // FR
        0.15f,                     // FC
        0.16f,                     // LFE
        0.13f,                     // BL
        0.14f,                     // BR
        lev * 0.11f + lev * 0.13f, // TML
        lev * 0.12f + lev * 0.14f, // TMR
        // 2
        0.21f,                     // FL
        0.22f,                     // FR
        0.25f,                     // FC
        0.26f,                     // LFE
        0.23f,                     // BL
        0.24f,                     // BR
        lev * 0.21f + lev * 0.23f, // TML
        lev * 0.22f + lev * 0.24f, // TMR
        // 3
        0.31f,                     // FL
        0.32f,                     // FR
        0.35f,                     // FC
        0.36f,                     // LFE
        0.33f,                     // BL
        0.34f,                     // BR
        lev * 0.31f + lev * 0.33f, // TML
        lev * 0.32f + lev * 0.34f, // TMR
        // 4
        0.41f,                     // FL
        0.42f,                     // FR
        0.45f,                     // FC
        0.46f,                     // LFE
        0.43f,                     // BL
        0.44f,                     // BR
        lev * 0.41f + lev * 0.43f, // TML
        lev * 0.42f + lev * 0.44f, // TMR
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Alsa, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// copy first channel from input, set rest to zero
TEST(channel_mapper, mono_to_multitrack) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_Mono, // FC
        OutChans = 0x88                   // C3, C7
    };

    const sample_t input[NumSamples] = {
        // FC
        0.01f, // 0
        0.02f, // 1
        0.03f, // 2
        0.04f, // 3
        0.05f, // 4
    };

    const sample_t output[NumSamples * 2] = {
        // C3     C7
        0.01f, 0.00f, // 0
        0.02f, 0.00f, // 1
        0.03f, 0.00f, // 2
        0.04f, 0.00f, // 3
        0.05f, 0.00f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Multitrack, ChanOrder_None, OutChans);
}

// copy first two channels from input, set rest to zero
TEST(channel_mapper, stereo_to_multitrack) {
    enum {
        NumSamples = 5,
        InChans = ChanMask_Surround_Stereo, // FL, FR
        OutChans = 0x888                    // C3, C7, C11
    };

    const sample_t input[NumSamples * 2] = {
        // FL      FR
        0.01f, -0.01f, // 0
        0.02f, -0.02f, // 1
        0.03f, -0.03f, // 2
        0.04f, -0.04f, // 3
        0.05f, -0.05f, // 4
    };

    const sample_t output[NumSamples * 3] = {
        // C3      C7    C11
        0.01f, -0.01f, 0.00f, // 0
        0.02f, -0.02f, 0.00f, // 1
        0.03f, -0.03f, 0.00f, // 2
        0.04f, -0.04f, 0.00f, // 3
        0.05f, -0.05f, 0.00f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Surround, ChanOrder_Smpte, InChans,
          ChanLayout_Multitrack, ChanOrder_None, OutChans);
}

// copy first channel to output, ignore rest
TEST(channel_mapper, multitrack_to_mono) {
    enum {
        NumSamples = 5,
        InChans = 0x88,                   // C3, C7
        OutChans = ChanMask_Surround_Mono // FC
    };

    const sample_t input[NumSamples * 2] = {
        // C3      C7
        0.01f, -0.01f, // 0
        0.02f, -0.02f, // 1
        0.03f, -0.03f, // 2
        0.04f, -0.04f, // 3
        0.05f, -0.05f, // 4
    };

    const sample_t output[NumSamples] = {
        // FC
        0.01f, // 0
        0.02f, // 1
        0.03f, // 2
        0.04f, // 3
        0.05f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Multitrack, ChanOrder_None, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// copy first two channels to output, ignore rest
TEST(channel_mapper, multitrack_to_stereo) {
    enum {
        NumSamples = 5,
        InChans = 0x888,                    // C3, C7, C11
        OutChans = ChanMask_Surround_Stereo // FL, FR
    };

    const sample_t input[NumSamples * 3] = {
        // C3      C7    C11
        0.01f, -0.01f, 0.33f, // 0
        0.02f, -0.02f, 0.33f, // 1
        0.03f, -0.03f, 0.33f, // 2
        0.04f, -0.04f, 0.33f, // 3
        0.05f, -0.05f, 0.33f, // 4
    };

    const sample_t output[NumSamples * 2] = {
        // FL      FR
        0.01f, -0.01f, // 0
        0.02f, -0.02f, // 1
        0.03f, -0.03f, // 2
        0.04f, -0.04f, // 3
        0.05f, -0.05f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Multitrack, ChanOrder_None, InChans,
          ChanLayout_Surround, ChanOrder_Smpte, OutChans);
}

// verbatim copy
TEST(channel_mapper, multitrack_same) {
    enum {
        NumSamples = 5,
        InChans = 0x3, // C0, C1
        OutChans = 0x3 // C0, C1
    };

    const sample_t input[NumSamples * 2] = {
        // C0     C1
        0.01f, 0.02f, // 0
        0.03f, 0.04f, // 1
        0.05f, 0.06f, // 2
        0.07f, 0.08f, // 3
        0.09f, 1.00f, // 4
    };

    const sample_t output[NumSamples * 2] = {
        // C0     C1
        0.01f, 0.02f, // 0
        0.03f, 0.04f, // 1
        0.05f, 0.06f, // 2
        0.07f, 0.08f, // 3
        0.09f, 1.00f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Multitrack, ChanOrder_None, InChans,
          ChanLayout_Multitrack, ChanOrder_None, OutChans);
}

// input mask is subset of output mask
TEST(channel_mapper, multitrack_subset) {
    enum {
        NumSamples = 5,
        InChans = 0x2, // C1
        OutChans = 0x3 // C0, C1
    };

    const sample_t input[NumSamples] = {
        // C1
        0.01f, // 0
        0.02f, // 1
        0.03f, // 2
        0.04f, // 3
        0.05f, // 4
    };

    const sample_t output[NumSamples * 2] = {
        // C0     C1
        0.00f, 0.01f, // 0
        0.00f, 0.02f, // 1
        0.00f, 0.03f, // 2
        0.00f, 0.04f, // 3
        0.00f, 0.05f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Multitrack, ChanOrder_None, InChans,
          ChanLayout_Multitrack, ChanOrder_None, OutChans);
}

// input mask is superset of output mask
TEST(channel_mapper, multitrack_superset) {
    enum {
        NumSamples = 5,
        InChans = 0x7, // C0, C1, C2
        OutChans = 0x3 // C0, C1
    };

    const sample_t input[NumSamples * 3] = {
        //  C0     C1     C2
        -0.01f, 0.01f, 0.08f, //
        -0.02f, 0.02f, 0.08f, //
        -0.03f, 0.03f, 0.08f, //
        -0.04f, 0.04f, 0.08f, //
        -0.05f, 0.05f, 0.08f, //
    };

    const sample_t output[NumSamples * 2] = {
        //  C0     C1
        -0.01f, 0.01f, //
        -0.02f, 0.02f, //
        -0.03f, 0.03f, //
        -0.04f, 0.04f, //
        -0.05f, 0.05f, //
    };

    check(input, output, NumSamples, ChanLayout_Multitrack, ChanOrder_None, InChans,
          ChanLayout_Multitrack, ChanOrder_None, OutChans);
}

// input and output masks overlap
TEST(channel_mapper, multitrack_overlap) {
    enum {
        NumSamples = 5,
        InChans = 0x5, // C0, C2
        OutChans = 0x3 // C0, C1
    };

    const sample_t input[NumSamples * 3] = {
        //  C0     C2
        -0.01f, 0.08f, // 0
        -0.02f, 0.08f, // 1
        -0.03f, 0.08f, // 2
        -0.04f, 0.08f, // 3
        -0.05f, 0.08f, // 4
    };

    const sample_t output[NumSamples * 2] = {
        //  C0     C1
        -0.01f, 0.00f, // 0
        -0.02f, 0.00f, // 1
        -0.03f, 0.00f, // 2
        -0.04f, 0.00f, // 3
        -0.05f, 0.00f, // 4
    };

    check(input, output, NumSamples, ChanLayout_Multitrack, ChanOrder_None, InChans,
          ChanLayout_Multitrack, ChanOrder_None, OutChans);
}

} // namespace audio
} // namespace roc
