/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/channel_mapper_table.h"
#include "roc_core/macro_helpers.h"

namespace roc {
namespace audio {

namespace {

static ChannelMask all_masks[] = {
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

static ChannelMask mapped_masks[] = {
    ChanMask_Surround_Mono,  //
    ChanMask_Surround_2_1,   //
    ChanMask_Surround_3_1,   //
    ChanMask_Surround_4_1,   //
    ChanMask_Surround_5_1_2, //
    ChanMask_Surround_5_1_4, //
    ChanMask_Surround_6_1,   //
    ChanMask_Surround_7_1_2, //
    ChanMask_Surround_7_1_4, //
};

int sortpos(ChannelMask ch_mask) {
    if (ch_mask == 0) {
        return 0;
    }
    for (size_t i = 0; i < ROC_ARRAY_SIZE(all_masks); i++) {
        if (ch_mask == all_masks[i]) {
            return (int)i + 1;
        }
    }
    FAIL("unknown mask");
    return -1;
}

void fail(const char* message, const ChannelMap& ch_map) {
    char buf[128] = {};
    snprintf(buf, sizeof(buf), "%s: mapping %s", message, ch_map.name);
    FAIL(buf);
}

} // namespace

TEST_GROUP(channel_mapper_table) {};

TEST(channel_mapper_table, masks) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(chan_maps); n++) {
        bool found_in = false, found_out = false;

        for (size_t i = 0; i < ROC_ARRAY_SIZE(mapped_masks); i++) {
            if (chan_maps[n].in_mask == mapped_masks[i]) {
                found_in = true;
            }
        }

        for (size_t i = 0; i < ROC_ARRAY_SIZE(all_masks); i++) {
            if (chan_maps[n].out_mask == all_masks[i]) {
                found_out = true;
            }
        }

        if (!found_in) {
            fail("unexpected input mask", chan_maps[n]);
        }

        if (!found_out) {
            fail("unexpected output mask", chan_maps[n]);
        }
    }
}

TEST(channel_mapper_table, combinations) {
    for (size_t i = 1; i < ROC_ARRAY_SIZE(mapped_masks); i++) {
        for (size_t j = 0; j < i; j++) {
            bool found = false;

            for (size_t n = 0; n < ROC_ARRAY_SIZE(chan_maps); n++) {
                if (chan_maps[n].in_mask == mapped_masks[i]
                    && chan_maps[n].out_mask == mapped_masks[j]) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                char buf[128] = {};
                snprintf(buf, sizeof(buf),
                         "mask combination not covered: input=%u output=%u", (unsigned)i,
                         (unsigned)j);
                FAIL(buf);
            }
        }
    }
}

TEST(channel_mapper_table, sorting) {
    ChannelMask in_mask = 0;
    ChannelMask out_mask = 0;

    for (size_t n = 0; n < ROC_ARRAY_SIZE(chan_maps); n++) {
        if (sortpos(chan_maps[n].in_mask) < sortpos(in_mask)) {
            fail("unexpected mapping order (input mask is before previous)",
                 chan_maps[n]);
        }

        if (in_mask == chan_maps[n].in_mask) {
            if (sortpos(chan_maps[n].out_mask) < sortpos(out_mask)) {
                fail("unexpected mapping order (output mask is before previous)",
                     chan_maps[n]);
            }
        }

        in_mask = chan_maps[n].in_mask;
        out_mask = chan_maps[n].out_mask;
    }
}

TEST(channel_mapper_table, channels) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(chan_maps); n++) {
        bool has_pair[ChanPos_Max][ChanPos_Max] = {};
        bool found_zero = false;

        for (size_t r = 0; r < ROC_ARRAY_SIZE(chan_maps[n].rules); r++) {
            const ChannelMapRule& rule = chan_maps[n].rules[r];

            if (rule.coeff == 0.f) {
                found_zero = true;
            }

            if (found_zero) {
                if (rule.coeff != 0.f) {
                    fail("unexpected non-zero coefficient", chan_maps[n]);
                }
                if (rule.out_ch != 0 || rule.in_ch != 0) {
                    fail("unexpected non-zero channel", chan_maps[n]);
                }
            } else {
                if (rule.out_ch < 0 || rule.out_ch >= ChanPos_Max) {
                    fail("output channel out of bounds", chan_maps[n]);
                }
                if (rule.in_ch < 0 || rule.in_ch >= ChanPos_Max) {
                    fail("input channel out of bounds", chan_maps[n]);
                }

                if (((1u << rule.out_ch) & chan_maps[n].out_mask) == 0) {
                    fail("output channel not present in output mask", chan_maps[n]);
                }
                if (((1u << rule.in_ch) & chan_maps[n].in_mask) == 0) {
                    fail("input channel not present in input mask", chan_maps[n]);
                }

                if (has_pair[rule.out_ch][rule.in_ch]) {
                    fail("multiple rules redefine same channel combination",
                         chan_maps[n]);
                }

                has_pair[rule.out_ch][rule.in_ch] = true;
            }
        }
    }
}

TEST(channel_mapper_table, completeness) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(chan_maps); n++) {
        ChannelMask actual_in_chans = 0;
        ChannelMask actual_out_chans = 0;

        for (size_t r = 0; r < ROC_ARRAY_SIZE(chan_maps[n].rules); r++) {
            const ChannelMapRule& rule = chan_maps[n].rules[r];

            if (rule.coeff != 0.f) {
                actual_in_chans |= (1u << rule.in_ch);
                actual_out_chans |= (1u << rule.out_ch);
            }
        }

        ChannelMask expected_in_mask = chan_maps[n].in_mask;
        ChannelMask expected_out_mask = chan_maps[n].out_mask;

        if ((expected_out_mask & (1u << ChanPos_LowFrequency)) == 0) {
            expected_in_mask &= ~(1u << ChanPos_LowFrequency);
        }

        if (actual_in_chans != expected_in_mask) {
            fail("unexpected input channels found in mapping", chan_maps[n]);
        }

        if (actual_out_chans != expected_out_mask) {
            fail("unexpected output channels found in mapping", chan_maps[n]);
        }
    }
}

TEST(channel_mapper_table, orders) {
    for (int n = 0; n < ChanOrder_Max; n++) {
        CHECK(n >= ChanOrder_None);
        CHECK(n < ChanOrder_Max);

        const ChannelOrder order = (ChannelOrder)n;
        const ChannelList& chan_list = chan_orders[order];

        size_t n_chans = 0;
        while (chan_list.chans[n_chans] != ChanPos_Max) {
            n_chans++;
        }

        if (order == ChanOrder_None) {
            CHECK(n_chans == 0);
        } else {
            CHECK(n_chans > 0);
            CHECK(n_chans <= ChanPos_Max);
        }

        for (size_t i = 0; i < n_chans; i++) {
            for (size_t j = i + 1; j < n_chans; j++) {
                CHECK(chan_list.chans[i] != chan_list.chans[j]);
            }
        }
    }
}

} // namespace audio
} // namespace roc
