/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/channel_defs.h"
#include "roc_audio/channel_tables.h"
#include "roc_core/macro_helpers.h"

namespace roc {
namespace audio {

namespace {

static ChannelMask all_masks[] = {
    ChanMask_Surround_Mono,     //
    ChanMask_Surround_1_1,      //
    ChanMask_Surround_1_1_3c,   //
    ChanMask_Surround_Stereo,   //
    ChanMask_Surround_2_1,      //
    ChanMask_Surround_3_0,      //
    ChanMask_Surround_3_1,      //
    ChanMask_Surround_3_1_3c,   //
    ChanMask_Surround_4_0,      //
    ChanMask_Surround_4_1,      //
    ChanMask_Surround_5_0,      //
    ChanMask_Surround_5_1,      //
    ChanMask_Surround_5_1_3c,   //
    ChanMask_Surround_5_1_2,    //
    ChanMask_Surround_5_1_2_3c, //
    ChanMask_Surround_5_1_4,    //
    ChanMask_Surround_5_1_4_3c, //
    ChanMask_Surround_6_0,      //
    ChanMask_Surround_6_1,      //
    ChanMask_Surround_6_1_3c,   //
    ChanMask_Surround_7_0,      //
    ChanMask_Surround_7_1,      //
    ChanMask_Surround_7_1_3c,   //
    ChanMask_Surround_7_1_2,    //
    ChanMask_Surround_7_1_2_3c, //
    ChanMask_Surround_7_1_4,    //
    ChanMask_Surround_7_1_4_3c, //
};

static ChannelMask named_masks[] = {
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

void fail(const char* message, const ChannelMapTable& ch_map) {
    char buf[128] = {};
    snprintf(buf, sizeof(buf), "%s: mapping %s", message, ch_map.name);
    FAIL(buf);
}

} // namespace

TEST_GROUP(channel_tables) {};

// Check that all masks in mapping tables are valid.
TEST(channel_tables, map_tables_masks) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(ChanMapTables); n++) {
        bool found_in = false, found_out = false;

        for (size_t i = 0; i < ROC_ARRAY_SIZE(all_masks); i++) {
            if (ChanMapTables[n].in_mask == all_masks[i]) {
                found_in = true;
            }
        }

        for (size_t i = 0; i < ROC_ARRAY_SIZE(all_masks); i++) {
            if (ChanMapTables[n].out_mask == all_masks[i]) {
                found_out = true;
            }
        }

        if (!found_in) {
            fail("unexpected input mask", ChanMapTables[n]);
        }

        if (!found_out) {
            fail("unexpected output mask", ChanMapTables[n]);
        }
    }
}

// Check that mapping tables are sorted correctly.
TEST(channel_tables, map_tables_sorting) {
    ChannelMask in_mask = 0;
    ChannelMask out_mask = 0;

    for (size_t n = 0; n < ROC_ARRAY_SIZE(ChanMapTables); n++) {
        if (sortpos(ChanMapTables[n].in_mask) < sortpos(in_mask)) {
            fail("unexpected mapping order (input mask is before previous)",
                 ChanMapTables[n]);
        }

        if (in_mask == ChanMapTables[n].in_mask) {
            if (sortpos(ChanMapTables[n].out_mask) < sortpos(out_mask)) {
                fail("unexpected mapping order (output mask is before previous)",
                     ChanMapTables[n]);
            }
        }

        in_mask = ChanMapTables[n].in_mask;
        out_mask = ChanMapTables[n].out_mask;
    }
}

// Check that rules of mapping tables use valid channels.
TEST(channel_tables, map_tables_channels) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(ChanMapTables); n++) {
        bool has_pair[ChanPos_Max][ChanPos_Max] = {};
        bool found_zero = false;

        for (size_t r = 0; r < ROC_ARRAY_SIZE(ChanMapTables[n].rules); r++) {
            const ChannelMapRule& rule = ChanMapTables[n].rules[r];

            if (rule.coeff == 0.f) {
                found_zero = true;
            }

            if (found_zero) {
                if (rule.coeff != 0.f) {
                    fail("unexpected non-zero coefficient", ChanMapTables[n]);
                }
                if (rule.out_ch != 0 || rule.in_ch != 0) {
                    fail("unexpected non-zero channel", ChanMapTables[n]);
                }
            } else {
                if (rule.out_ch < 0 || rule.out_ch >= ChanPos_Max) {
                    fail("output channel out of bounds", ChanMapTables[n]);
                }
                if (rule.in_ch < 0 || rule.in_ch >= ChanPos_Max) {
                    fail("input channel out of bounds", ChanMapTables[n]);
                }

                if (((1u << rule.out_ch) & ChanMapTables[n].out_mask) == 0) {
                    fail("output channel not present in output mask", ChanMapTables[n]);
                }
                if (((1u << rule.in_ch) & ChanMapTables[n].in_mask) == 0) {
                    fail("input channel not present in input mask", ChanMapTables[n]);
                }

                if (has_pair[rule.out_ch][rule.in_ch]) {
                    fail("multiple rules redefine same channel combination",
                         ChanMapTables[n]);
                }

                has_pair[rule.out_ch][rule.in_ch] = true;
            }
        }
    }
}

// Check that rules of mapping tables use all channels in mask.
TEST(channel_tables, map_tables_completeness) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(ChanMapTables); n++) {
        ChannelMask actual_in_chans = 0;
        ChannelMask actual_out_chans = 0;

        for (size_t r = 0; r < ROC_ARRAY_SIZE(ChanMapTables[n].rules); r++) {
            const ChannelMapRule& rule = ChanMapTables[n].rules[r];

            if (rule.coeff != 0.f) {
                actual_in_chans |= (1u << rule.in_ch);
                actual_out_chans |= (1u << rule.out_ch);
            }
        }

        ChannelMask expected_in_mask = ChanMapTables[n].in_mask;
        ChannelMask expected_out_mask = ChanMapTables[n].out_mask;

        if ((expected_out_mask & (1u << ChanPos_LowFrequency)) == 0) {
            expected_in_mask &= ~(1u << ChanPos_LowFrequency);
        }

        if (actual_in_chans != expected_in_mask) {
            fail("unexpected input channels found in mapping", ChanMapTables[n]);
        }

        if (actual_out_chans != expected_out_mask) {
            fail("unexpected output channels found in mapping", ChanMapTables[n]);
        }
    }
}

// Check validity of order tables.
TEST(channel_tables, order_tables) {
    for (int n = 0; n < ChanOrder_Max; n++) {
        const ChannelOrder order = (ChannelOrder)n;

        LONGS_EQUAL(order, ChanOrderTables[n].order);
        CHECK(ChanOrderTables[n].name);

        size_t n_chans = 0;
        while (ChanOrderTables[order].chans[n_chans] != ChanPos_Max) {
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
                CHECK(ChanOrderTables[order].chans[i] != ChanOrderTables[order].chans[j]);
            }
        }
    }
}

// Check validity of name tables.
TEST(channel_tables, name_tables) {
    LONGS_EQUAL(ROC_ARRAY_SIZE(ChanPositionNames), ChanPos_Max);

    for (int n = 0; n < ChanPos_Max; n++) {
        const ChannelPosition pos = (ChannelPosition)n;

        LONGS_EQUAL(pos, ChanPositionNames[n].pos);
        CHECK(ChanPositionNames[n].name);
    }

    LONGS_EQUAL(ROC_ARRAY_SIZE(ChanMaskNames), ROC_ARRAY_SIZE(named_masks));

    for (size_t msk = 0; msk < ROC_ARRAY_SIZE(named_masks); msk++) {
        int found = 0;

        for (size_t n = 0; n < ROC_ARRAY_SIZE(ChanMaskNames); n++) {
            CHECK(ChanMaskNames[n].mask);
            CHECK(ChanMaskNames[n].name);

            if (ChanMaskNames[n].mask == named_masks[msk]) {
                found++;
            }
        }

        LONGS_EQUAL(1, found);
    }
}

// Check that we can retrieve all names.
TEST(channel_tables, name_functions) {
    for (int n = 0; n < ChanOrder_Max; n++) {
        CHECK(channel_order_to_str((ChannelOrder)n));
        STRCMP_EQUAL(ChanOrderTables[n].name, channel_order_to_str((ChannelOrder)n));
    }

    for (int n = 0; n < ChanPos_Max; n++) {
        CHECK(channel_pos_to_str((ChannelPosition)n));
        STRCMP_EQUAL(ChanPositionNames[n].name, channel_pos_to_str((ChannelPosition)n));
    }

    for (size_t n = 0; n < ROC_ARRAY_SIZE(named_masks); n++) {
        CHECK(channel_mask_to_str(named_masks[n]));
    }
}

} // namespace audio
} // namespace roc
