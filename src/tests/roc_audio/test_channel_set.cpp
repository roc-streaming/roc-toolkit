/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/channel_defs.h"
#include "roc_audio/channel_set.h"
#include "roc_audio/channel_set_to_str.h"

namespace roc {
namespace audio {

TEST_GROUP(channel_set) {};

TEST(channel_set, layout) {
    ChannelSet ch_set;

    LONGS_EQUAL(ChanLayout_None, ch_set.layout());

    ch_set.set_layout(ChanLayout_Surround);

    LONGS_EQUAL(ChanLayout_Surround, ch_set.layout());
}

TEST(channel_set, order) {
    ChannelSet ch_set;

    LONGS_EQUAL(ChanOrder_None, ch_set.order());

    ch_set.set_order(ChanOrder_Smpte);

    LONGS_EQUAL(ChanOrder_Smpte, ch_set.order());
}

TEST(channel_set, empty) {
    ChannelSet ch_set;

    CHECK(ch_set.max_channels() >= 256);

    UNSIGNED_LONGS_EQUAL(0, ch_set.num_channels());

    for (size_t n = 0; n < ch_set.max_channels(); n++) {
        CHECK(!ch_set.has_channel(n));
    }
}

TEST(channel_set, set_mask) {
    { // set
        ChannelSet ch_set;

        ch_set.set_mask((ChannelMask(1) << 11) | (ChannelMask(1) << 22));

        UNSIGNED_LONGS_EQUAL(2, ch_set.num_channels());

        for (size_t n = 0; n < ch_set.max_channels(); n++) {
            if (n == 11 || n == 22) {
                CHECK(ch_set.has_channel(n));
            } else {
                CHECK(!ch_set.has_channel(n));
            }
        }

        UNSIGNED_LONGS_EQUAL(11, ch_set.first_channel());
        UNSIGNED_LONGS_EQUAL(22, ch_set.last_channel());
    }
    { // overwrite
        ChannelSet ch_set;

        ch_set.toggle_channel(12, true);
        ch_set.toggle_channel(100, true);

        UNSIGNED_LONGS_EQUAL(2, ch_set.num_channels());

        for (size_t n = 0; n < ch_set.max_channels(); n++) {
            if (n == 12 || n == 100) {
                CHECK(ch_set.has_channel(n));
            } else {
                CHECK(!ch_set.has_channel(n));
            }
        }

        ch_set.set_mask((ChannelMask(1) << 11) | (ChannelMask(1) << 22));

        UNSIGNED_LONGS_EQUAL(2, ch_set.num_channels());

        for (size_t n = 0; n < ch_set.max_channels(); n++) {
            if (n == 11 || n == 22) {
                CHECK(ch_set.has_channel(n));
            } else {
                CHECK(!ch_set.has_channel(n));
            }
        }

        UNSIGNED_LONGS_EQUAL(11, ch_set.first_channel());
        UNSIGNED_LONGS_EQUAL(22, ch_set.last_channel());
    }
    { // construct
        ChannelSet ch_set(ChanLayout_Surround, ChanOrder_Smpte, (1 << 11) | (1 << 22));

        LONGS_EQUAL(ChanLayout_Surround, ch_set.layout());

        UNSIGNED_LONGS_EQUAL(2, ch_set.num_channels());

        for (size_t n = 0; n < ch_set.max_channels(); n++) {
            if (n == 11 || n == 22) {
                CHECK(ch_set.has_channel(n));
            } else {
                CHECK(!ch_set.has_channel(n));
            }
        }

        UNSIGNED_LONGS_EQUAL(11, ch_set.first_channel());
        UNSIGNED_LONGS_EQUAL(22, ch_set.last_channel());
    }
}

TEST(channel_set, set_range) {
    { // set
        ChannelSet ch_set;

        ch_set.set_range(11, 111);

        UNSIGNED_LONGS_EQUAL(101, ch_set.num_channels());

        for (size_t n = 0; n < ch_set.max_channels(); n++) {
            if (n >= 11 && n <= 111) {
                CHECK(ch_set.has_channel(n));
            } else {
                CHECK(!ch_set.has_channel(n));
            }
        }

        UNSIGNED_LONGS_EQUAL(11, ch_set.first_channel());
        UNSIGNED_LONGS_EQUAL(111, ch_set.last_channel());
    }
    { // overwrite
        ChannelSet ch_set;

        ch_set.toggle_channel(11, true);
        ch_set.toggle_channel(100, true);

        UNSIGNED_LONGS_EQUAL(2, ch_set.num_channels());

        ch_set.set_range(20, 90);

        UNSIGNED_LONGS_EQUAL(71, ch_set.num_channels());

        size_t enabled = 0;

        for (size_t n = 0; n < ch_set.max_channels(); n++) {
            if (n >= 20 && n <= 90) {
                CHECK(ch_set.has_channel(n));
                enabled++;
            } else {
                CHECK(!ch_set.has_channel(n));
            }
        }

        UNSIGNED_LONGS_EQUAL(enabled, ch_set.num_channels());

        UNSIGNED_LONGS_EQUAL(20, ch_set.first_channel());
        UNSIGNED_LONGS_EQUAL(90, ch_set.last_channel());
    }
}

TEST(channel_set, set_count) {
    for (size_t count = 0; count < ChannelSet::max_channels(); count++) {
        ChannelSet ch_set;

        ch_set.set_count(count);

        UNSIGNED_LONGS_EQUAL(count, ch_set.num_channels());
    }
}

TEST(channel_set, toggle_channel) {
    { // set small
        ChannelSet ch_set;

        ch_set.toggle_channel(0, true);
        ch_set.toggle_channel(5, true);

        UNSIGNED_LONGS_EQUAL(2, ch_set.num_channels());

        for (size_t n = 0; n < ch_set.max_channels(); n++) {
            if (n == 0 || n == 5) {
                CHECK(ch_set.has_channel(n));
            } else {
                CHECK(!ch_set.has_channel(n));
            }
        }

        UNSIGNED_LONGS_EQUAL(0, ch_set.first_channel());
        UNSIGNED_LONGS_EQUAL(5, ch_set.last_channel());
    }
    { // set large
        ChannelSet ch_set;

        ch_set.toggle_channel(11, true);
        ch_set.toggle_channel(101, true);

        UNSIGNED_LONGS_EQUAL(2, ch_set.num_channels());

        for (size_t n = 0; n < ch_set.max_channels(); n++) {
            if (n == 11 || n == 101) {
                CHECK(ch_set.has_channel(n));
            } else {
                CHECK(!ch_set.has_channel(n));
            }
        }

        UNSIGNED_LONGS_EQUAL(11, ch_set.first_channel());
        UNSIGNED_LONGS_EQUAL(101, ch_set.last_channel());
    }
    { // unset
        ChannelSet ch_set;

        ch_set.toggle_channel(11, true);
        ch_set.toggle_channel(101, true);

        ch_set.toggle_channel(11, false);
        ch_set.toggle_channel(12, false);

        UNSIGNED_LONGS_EQUAL(1, ch_set.num_channels());

        for (size_t n = 0; n < ch_set.max_channels(); n++) {
            if (n == 101) {
                CHECK(ch_set.has_channel(n));
            } else {
                CHECK(!ch_set.has_channel(n));
            }
        }

        UNSIGNED_LONGS_EQUAL(101, ch_set.first_channel());
        UNSIGNED_LONGS_EQUAL(101, ch_set.last_channel());
    }
}

TEST(channel_set, toggle_channel_range) {
    { // set
        ChannelSet ch_set;

        ch_set.toggle_channel_range(11, 111, true);

        UNSIGNED_LONGS_EQUAL(101, ch_set.num_channels());

        for (size_t n = 0; n < ch_set.max_channels(); n++) {
            if (n >= 11 && n <= 111) {
                CHECK(ch_set.has_channel(n));
            } else {
                CHECK(!ch_set.has_channel(n));
            }
        }

        UNSIGNED_LONGS_EQUAL(11, ch_set.first_channel());
        UNSIGNED_LONGS_EQUAL(111, ch_set.last_channel());
    }
    { // unset
        ChannelSet ch_set;

        ch_set.toggle_channel_range(11, 111, true);
        ch_set.toggle_channel_range(70, 79, false);
        ch_set.toggle_channel_range(101, 120, false);

        UNSIGNED_LONGS_EQUAL(80, ch_set.num_channels());

        size_t enabled = 0;

        for (size_t n = 0; n < ch_set.max_channels(); n++) {
            if ((n >= 11 && n <= 69) || (n >= 80 && n <= 100)) {
                CHECK(ch_set.has_channel(n));
                enabled++;
            } else {
                CHECK(!ch_set.has_channel(n));
            }
        }

        UNSIGNED_LONGS_EQUAL(enabled, ch_set.num_channels());

        UNSIGNED_LONGS_EQUAL(11, ch_set.first_channel());
        UNSIGNED_LONGS_EQUAL(100, ch_set.last_channel());
    }
}

TEST(channel_set, is_valid) {
    { // no layout, no order, no channels (bad)
        ChannelSet ch_set;
        CHECK(!ch_set.is_valid());
    }
    { // no layout, no order, has channels (bad)
        ChannelSet ch_set;
        CHECK(!ch_set.is_valid());

        ch_set.toggle_channel(11, true);
        CHECK(!ch_set.is_valid());
    }
    { // no layout, has order, has channels (bad)
        ChannelSet ch_set;
        CHECK(!ch_set.is_valid());

        ch_set.set_order(ChanOrder_Smpte);
        CHECK(!ch_set.is_valid());

        ch_set.toggle_channel(11, true);
        CHECK(!ch_set.is_valid());
    }
    { // surround layout, no order, has channels (bad)
        ChannelSet ch_set;
        CHECK(!ch_set.is_valid());

        ch_set.set_layout(ChanLayout_Surround);
        CHECK(!ch_set.is_valid());

        ch_set.toggle_channel(11, true);
        CHECK(!ch_set.is_valid());
    }
    { // surround layout, has order, no channels (bad)
        ChannelSet ch_set;
        CHECK(!ch_set.is_valid());

        ch_set.set_layout(ChanLayout_Surround);
        CHECK(!ch_set.is_valid());

        ch_set.set_order(ChanOrder_Smpte);
        CHECK(!ch_set.is_valid());
    }
    { // surround layout, has order, channel out of bounds (bad)
        ChannelSet ch_set;
        CHECK(!ch_set.is_valid());

        ch_set.set_layout(ChanLayout_Surround);
        CHECK(!ch_set.is_valid());

        ch_set.set_order(ChanOrder_Smpte);
        CHECK(!ch_set.is_valid());

        ch_set.toggle_channel(100, true);
        CHECK(!ch_set.is_valid());
    }
    { // surround layout, has order, has channels (good)
        ChannelSet ch_set;
        CHECK(!ch_set.is_valid());

        ch_set.set_layout(ChanLayout_Surround);
        CHECK(!ch_set.is_valid());

        ch_set.set_order(ChanOrder_Smpte);
        CHECK(!ch_set.is_valid());

        ch_set.toggle_channel(11, true);
        CHECK(ch_set.is_valid());
    }
    { // multitrack layout, no order, no channels (bad)
        ChannelSet ch_set;
        CHECK(!ch_set.is_valid());

        ch_set.set_layout(ChanLayout_Multitrack);
        CHECK(!ch_set.is_valid());
    }
    { // multitrack layout, has order, has channels (bad)
        ChannelSet ch_set;
        CHECK(!ch_set.is_valid());

        ch_set.set_layout(ChanLayout_Multitrack);
        CHECK(!ch_set.is_valid());

        ch_set.set_order(ChanOrder_Smpte);
        CHECK(!ch_set.is_valid());

        ch_set.toggle_channel(11, true);
        CHECK(!ch_set.is_valid());
    }
    { // multitrack layout, no order, has channels (good)
        ChannelSet ch_set;
        CHECK(!ch_set.is_valid());

        ch_set.set_layout(ChanLayout_Multitrack);
        CHECK(!ch_set.is_valid());

        ch_set.toggle_channel(11, true);
        CHECK(ch_set.is_valid());
    }
}

TEST(channel_set, clear) {
    ChannelSet ch_set;

    ch_set.set_layout(ChanLayout_Surround);
    ch_set.set_order(ChanOrder_Smpte);

    ch_set.toggle_channel(11, true);

    CHECK(ch_set.is_valid());

    LONGS_EQUAL(ChanLayout_Surround, ch_set.layout());
    LONGS_EQUAL(ChanOrder_Smpte, ch_set.order());

    UNSIGNED_LONGS_EQUAL(1, ch_set.num_channels());

    ch_set.clear();

    CHECK(!ch_set.is_valid());

    LONGS_EQUAL(ChanLayout_None, ch_set.layout());
    LONGS_EQUAL(ChanOrder_None, ch_set.order());

    UNSIGNED_LONGS_EQUAL(0, ch_set.num_channels());

    for (size_t n = 0; n < ch_set.max_channels(); n++) {
        CHECK(!ch_set.has_channel(n));
    }
}

TEST(channel_set, equal_subset_superset) {
    { // empty
        ChannelSet ch_set;

        ch_set.set_layout(ChanLayout_Surround);

        CHECK(ch_set.is_equal(0x0));
        CHECK(ch_set.is_subset(0x0));
        CHECK(ch_set.is_superset(0x0));

        CHECK(!ch_set.is_equal(0xffffffff));
        CHECK(ch_set.is_subset(0xffffffff));
        CHECK(!ch_set.is_superset(0xffffffff));
    }
    { // normal
        ChannelSet ch_set;

        ch_set.set_layout(ChanLayout_Surround);
        ch_set.set_mask(0x5);

        CHECK(ch_set.is_equal(0x5));
        CHECK(ch_set.is_subset(0x5));
        CHECK(ch_set.is_superset(0x5));

        CHECK(!ch_set.is_equal(0x7));
        CHECK(ch_set.is_subset(0x7));
        CHECK(!ch_set.is_superset(0x7));

        CHECK(!ch_set.is_equal(0x4));
        CHECK(!ch_set.is_subset(0x4));
        CHECK(ch_set.is_superset(0x4));

        CHECK(!ch_set.is_equal(0x0));
        CHECK(!ch_set.is_subset(0x0));
        CHECK(ch_set.is_superset(0x0));

        CHECK(!ch_set.is_equal(0xffffffff));
        CHECK(ch_set.is_subset(0xffffffff));
        CHECK(!ch_set.is_superset(0xffffffff));
    }
    { // large
        ChannelSet ch_set;

        ch_set.toggle_channel(2, true);
        ch_set.toggle_channel(101, true);

        CHECK(!ch_set.is_equal(0x2));
        CHECK(!ch_set.is_subset(0x2));
        CHECK(ch_set.is_superset(0x2));

        CHECK(!ch_set.is_equal(0x0));
        CHECK(!ch_set.is_subset(0x0));
        CHECK(ch_set.is_superset(0x0));

        CHECK(!ch_set.is_equal(0xffffffff));
        CHECK(!ch_set.is_subset(0xffffffff));
        CHECK(ch_set.is_superset(0xffffffff));
    }
}

TEST(channel_set, bitwise_and) {
    ChannelSet ch_set;

    ch_set.toggle_channel(10, true);
    ch_set.toggle_channel(100, true);

    {
        ChannelSet other_set;

        other_set.toggle_channel(100, true);
        other_set.toggle_channel(200, true);

        ch_set.bitwise_and(other_set);
    }

    UNSIGNED_LONGS_EQUAL(1, ch_set.num_channels());

    for (size_t n = 0; n < ch_set.max_channels(); n++) {
        if (n == 100) {
            CHECK(ch_set.has_channel(n));
        } else {
            CHECK(!ch_set.has_channel(n));
        }
    }

    UNSIGNED_LONGS_EQUAL(100, ch_set.first_channel());
    UNSIGNED_LONGS_EQUAL(100, ch_set.last_channel());
}

TEST(channel_set, bitwise_or) {
    ChannelSet ch_set;

    ch_set.toggle_channel(10, true);
    ch_set.toggle_channel(100, true);

    {
        ChannelSet other_set;

        other_set.toggle_channel(100, true);
        other_set.toggle_channel(200, true);

        ch_set.bitwise_or(other_set);
    }

    UNSIGNED_LONGS_EQUAL(3, ch_set.num_channels());

    for (size_t n = 0; n < ch_set.max_channels(); n++) {
        if (n == 10 || n == 100 || n == 200) {
            CHECK(ch_set.has_channel(n));
        } else {
            CHECK(!ch_set.has_channel(n));
        }
    }

    UNSIGNED_LONGS_EQUAL(10, ch_set.first_channel());
    UNSIGNED_LONGS_EQUAL(200, ch_set.last_channel());
}

TEST(channel_set, bitwise_xor) {
    ChannelSet ch_set;

    ch_set.toggle_channel(10, true);
    ch_set.toggle_channel(100, true);

    {
        ChannelSet other_set;

        other_set.toggle_channel(100, true);
        other_set.toggle_channel(200, true);

        ch_set.bitwise_xor(other_set);
    }

    UNSIGNED_LONGS_EQUAL(2, ch_set.num_channels());

    for (size_t n = 0; n < ch_set.max_channels(); n++) {
        if (n == 10 || n == 200) {
            CHECK(ch_set.has_channel(n));
        } else {
            CHECK(!ch_set.has_channel(n));
        }
    }

    UNSIGNED_LONGS_EQUAL(10, ch_set.first_channel());
    UNSIGNED_LONGS_EQUAL(200, ch_set.last_channel());
}

TEST(channel_set, to_string) {
    {
        ChannelSet ch_set;

        STRCMP_EQUAL("<none 0 none>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set;
        ch_set.set_layout(ChanLayout_Surround);
        ch_set.set_order(ChanOrder_Smpte);

        STRCMP_EQUAL("<surround smpte 0 none>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set(ChanLayout_Surround, ChanOrder_Smpte, ChanMask_Surround_Mono);

        STRCMP_EQUAL("<surround smpte 1 FC>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set(ChanLayout_Surround, ChanOrder_Smpte, ChanMask_Surround_Stereo);

        STRCMP_EQUAL("<surround smpte 2 FL,FR>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set(ChanLayout_Surround, ChanOrder_Alsa, ChanMask_Surround_Stereo);

        STRCMP_EQUAL("<surround alsa 2 FL,FR>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set;
        ch_set.set_layout(ChanLayout_Multitrack);

        STRCMP_EQUAL("<multitrack 0 none>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set;
        ch_set.set_layout(ChanLayout_Multitrack);
        ch_set.set_range(0, 7);

        STRCMP_EQUAL("<multitrack 8 0xFF>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set;
        ch_set.set_layout(ChanLayout_Multitrack);
        ch_set.toggle_channel(2, true);
        ch_set.toggle_channel(3, true);
        ch_set.toggle_channel(5, true);
        ch_set.toggle_channel(7, true);

        STRCMP_EQUAL("<multitrack 4 0xAC>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set;
        ch_set.set_layout(ChanLayout_Multitrack);
        ch_set.toggle_channel(2, true);
        ch_set.toggle_channel(3, true);
        ch_set.toggle_channel(85, true);
        ch_set.toggle_channel(87, true);

        STRCMP_EQUAL("<multitrack 4 0xA00000000000000000000C>",
                     channel_set_to_str(ch_set).c_str());
    }
}

} // namespace audio
} // namespace roc
