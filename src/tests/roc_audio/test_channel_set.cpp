/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/channel_set.h"
#include "roc_audio/channel_set_to_str.h"

namespace roc {
namespace audio {

TEST_GROUP(channel_set) {};

TEST(channel_set, layout) {
    ChannelSet ch_set;

    LONGS_EQUAL(ChannelLayout_Invalid, ch_set.layout());

    ch_set.set_layout(ChannelLayout_Surround);

    LONGS_EQUAL(ChannelLayout_Surround, ch_set.layout());
}

TEST(channel_set, empty) {
    ChannelSet ch_set;

    CHECK(ch_set.max_channels() >= 256);

    UNSIGNED_LONGS_EQUAL(0, ch_set.num_channels());

    for (size_t n = 0; n < ch_set.max_channels(); n++) {
        CHECK(!ch_set.has_channel(n));
    }
}

TEST(channel_set, set_channel_small) {
    ChannelSet ch_set;

    ch_set.set_channel(0, true);
    ch_set.set_channel(5, true);

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

TEST(channel_set, set_channel_large) {
    ChannelSet ch_set;

    ch_set.set_channel(11, true);
    ch_set.set_channel(101, true);

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

TEST(channel_set, unset_channel) {
    ChannelSet ch_set;

    ch_set.set_channel(11, true);
    ch_set.set_channel(101, true);

    ch_set.set_channel(11, false);
    ch_set.set_channel(12, false);

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

TEST(channel_set, set_channel_range) {
    ChannelSet ch_set;

    ch_set.set_channel_range(11, 111, true);

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

TEST(channel_set, unset_channel_range) {
    ChannelSet ch_set;

    ch_set.set_channel_range(11, 111, true);
    ch_set.set_channel_range(70, 79, false);
    ch_set.set_channel_range(101, 120, false);

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

TEST(channel_set, set_channel_mask) {
    ChannelSet ch_set;

    ch_set.set_channel_mask((ChannelMask(1) << 11) | (ChannelMask(1) << 22));

    CHECK(ch_set.has_channel_mask((ChannelMask(1) << 11) | (ChannelMask(1) << 22)));

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

TEST(channel_set, overwrite_channel_mask) {
    ChannelSet ch_set;

    ch_set.set_channel(12, true);
    ch_set.set_channel(100, true);

    CHECK(!ch_set.has_channel_mask(ChannelMask(1) << 11));

    ch_set.set_channel_mask((ChannelMask(1) << 11) | (ChannelMask(1) << 22));

    CHECK(ch_set.has_channel_mask((ChannelMask(1) << 11) | (ChannelMask(1) << 22)));

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

TEST(channel_set, construct_channel_mask) {
    ChannelSet ch_set(ChannelLayout_Surround, (1 << 11) | (1 << 22));

    LONGS_EQUAL(ChannelLayout_Surround, ch_set.layout());

    CHECK(ch_set.has_channel_mask((ChannelMask(1) << 11) | (ChannelMask(1) << 22)));

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

TEST(channel_set, bitwise_and) {
    ChannelSet ch_set;

    ch_set.set_channel(10, true);
    ch_set.set_channel(100, true);

    {
        ChannelSet other_set;

        other_set.set_channel(100, true);
        other_set.set_channel(200, true);

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

    ch_set.set_channel(10, true);
    ch_set.set_channel(100, true);

    {
        ChannelSet other_set;

        other_set.set_channel(100, true);
        other_set.set_channel(200, true);

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

    ch_set.set_channel(10, true);
    ch_set.set_channel(100, true);

    {
        ChannelSet other_set;

        other_set.set_channel(100, true);
        other_set.set_channel(200, true);

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

        STRCMP_EQUAL("<invalid n_ch=0>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set(ChannelLayout_Mono, ChannelMask_Mono);

        STRCMP_EQUAL("<mono n_ch=1 ch=0x1>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set(ChannelLayout_Surround, ChannelMask_Stereo);

        STRCMP_EQUAL("<surround n_ch=2 ch=L,R>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set;

        ch_set.set_layout(ChannelLayout_Multitrack);
        ch_set.set_channel_range(0, 7, true);

        STRCMP_EQUAL("<multitrack n_ch=8 ch=0xff>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set;

        ch_set.set_layout(ChannelLayout_Multitrack);
        ch_set.set_channel(4, true);
        ch_set.set_channel(7, true);
        ch_set.set_channel_range(8, 11, true);

        // 256-bit mask is formatted from LSB to MSB in hex
        // trailing zeros are truncated

        // channel:    0-3   4-7  8-11
        // bitmask:   0000  1001  1111
        // hex:        0x0   0x9   0xf

        STRCMP_EQUAL("<multitrack n_ch=6 ch=0x09f>", channel_set_to_str(ch_set).c_str());
    }
    {
        ChannelSet ch_set;

        ch_set.set_layout(ChannelLayout_Multitrack);
        ch_set.set_channel_range(68, 70, true);

        STRCMP_EQUAL("<multitrack n_ch=3 ch=0x000000000000000007>",
                     channel_set_to_str(ch_set).c_str());
    }
}

} // namespace audio
} // namespace roc
