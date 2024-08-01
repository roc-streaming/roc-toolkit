/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_stat/mov_histogram.h"

namespace roc {
namespace stat {

TEST_GROUP(mov_histogram) {
    core::HeapArena arena;
};

TEST(mov_histogram, single_pass) {
    const size_t value_range_min = 0;
    const size_t value_range_max = 100;
    const size_t num_bins = 10;
    const size_t win_length = 10;

    MovHistogram<size_t> hist(arena, value_range_min, value_range_max, num_bins,
                              win_length);
    CHECK(hist.is_valid());

    for (size_t i = 0; i < win_length; i++) {
        hist.add(i * num_bins);
    }

    for (size_t i = 0; i < num_bins; ++i) {
        LONGS_EQUAL(1, hist.mov_counter(i));
    }
}

TEST(mov_histogram, rolling_window) {
    const size_t value_range_min = 0;
    const size_t value_range_max = 100;
    const size_t num_bins = 10;
    const size_t win_length = 5;

    MovHistogram<size_t> hist(arena, value_range_min, value_range_max, num_bins,
                              win_length);
    CHECK(hist.is_valid());

    for (size_t i = 0; i < win_length * 2; i++) {
        hist.add(i * (value_range_max / num_bins));
    }

    for (size_t i = 0; i < num_bins; ++i) {
        LONGS_EQUAL(i < win_length ? 0 : 1, hist.mov_counter(i));
    }
}

TEST(mov_histogram, value_equal_to_value_range_max) {
    const size_t value_range_min = 0;
    const size_t value_range_max = 100;
    const size_t num_bins = 10;
    const size_t win_length = 10;

    MovHistogram<size_t> hist(arena, value_range_min, value_range_max, num_bins,
                              win_length);
    CHECK(hist.is_valid());

    size_t test_value = value_range_max;
    hist.add(test_value);

    size_t expected_bin_index = num_bins - 1;
    LONGS_EQUAL(1, hist.mov_counter(expected_bin_index));
}

TEST(mov_histogram, value_is_float) {
    const float value_range_min = 0.0f;
    const float value_range_max = 100.0f;
    const size_t num_bins = 10;
    const size_t win_length = 10;

    MovHistogram<float> hist(arena, value_range_min, value_range_max, num_bins,
                             win_length);
    CHECK(hist.is_valid());

    for (size_t i = 0; i < win_length; i++) {
        hist.add(i * num_bins * 1.0f);
    }

    for (size_t i = 0; i < num_bins; ++i) {
        LONGS_EQUAL(1, hist.mov_counter(i));
    }
}

TEST(mov_histogram, min_max_negative) {
    const int value_range_min = -150;
    const int value_range_max = -50;
    const size_t num_bins = 10;
    const size_t win_length = 10;

    MovHistogram<int> hist(arena, value_range_min, value_range_max, num_bins, win_length);
    CHECK(hist.is_valid());

    const int bin_width = (value_range_max - value_range_min) / (int)(num_bins);

    for (size_t i = 0; i < win_length; i++) {
        int value = value_range_min + (int)i * bin_width;
        hist.add(value);
    }

    for (size_t i = 0; i < num_bins; ++i) {
        LONGS_EQUAL(1, hist.mov_counter(i));
    }
}

TEST(mov_histogram, win_length_equal_one) {
    const size_t value_range_min = 0;
    const size_t value_range_max = 100;
    const size_t num_bins = 10;
    const size_t win_length = 1;

    MovHistogram<size_t> hist(arena, value_range_min, value_range_max, num_bins,
                              win_length);
    CHECK(hist.is_valid());

    hist.add(0);
    hist.add(10);
    hist.add(20);

    LONGS_EQUAL(0, hist.mov_counter(0));
    LONGS_EQUAL(0, hist.mov_counter(1));
    LONGS_EQUAL(1, hist.mov_counter(2));
}

TEST(mov_histogram, multiple_values_in_bins) {
    const size_t value_range_min = 0;
    const size_t value_range_max = 100;
    const size_t num_bins = 10;
    const size_t win_length = 50;

    MovHistogram<size_t> hist(arena, value_range_min, value_range_max, num_bins,
                              win_length);
    CHECK(hist.is_valid());

    const size_t values_per_bin = 5;
    const size_t total_values = num_bins * values_per_bin;

    for (size_t i = 0; i < total_values; ++i) {
        size_t value = (i / values_per_bin) * (value_range_max / num_bins);
        hist.add(value);
    }

    for (size_t i = 0; i < num_bins; ++i) {
        LONGS_EQUAL(values_per_bin, hist.mov_counter(i));
    }
}

TEST(mov_histogram, rolling_window_bin_changes) {
    const size_t value_range_min = 0;
    const size_t value_range_max = 100;
    const size_t num_bins = 10;
    const size_t win_length = 5;

    MovHistogram<size_t> hist(arena, value_range_min, value_range_max, num_bins,
                              win_length);
    CHECK(hist.is_valid());

    for (size_t i = 0; i < win_length; i++) {
        hist.add(i * (value_range_max / num_bins));
    }

    for (size_t i = 0; i < num_bins; i++) {
        LONGS_EQUAL(i < win_length ? 1 : 0, hist.mov_counter(i));
    }

    hist.add(win_length * (value_range_max / num_bins));

    for (size_t i = 0; i < num_bins; i++) {
        if (i < 1) {
            LONGS_EQUAL(0, hist.mov_counter(i));
        } else if (i <= win_length) {
            LONGS_EQUAL(1, hist.mov_counter(i));
        } else {
            LONGS_EQUAL(0, hist.mov_counter(i));
        }
    }
}

TEST(mov_histogram, clamping_values) {
    const size_t value_range_min = 50;
    const size_t value_range_max = 150;
    const size_t num_bins = 10;
    const size_t win_length = 10;

    MovHistogram<size_t> hist(arena, value_range_min, value_range_max, num_bins,
                              win_length);
    CHECK(hist.is_valid());

    hist.add(size_t(20));
    hist.add(size_t(5));
    hist.add(size_t(10));

    hist.add(size_t(60));
    hist.add(size_t(80));

    hist.add(size_t(160));
    hist.add(size_t(170));
    hist.add(size_t(180));

    LONGS_EQUAL(3, hist.mov_counter(0));
    LONGS_EQUAL(1, hist.mov_counter(1));
    LONGS_EQUAL(1, hist.mov_counter(3));
    LONGS_EQUAL(3, hist.mov_counter(9));
}

TEST(mov_histogram, quantile) {
    const size_t value_range_min = 0;
    const size_t value_range_max = 100;
    const size_t num_bins = 10;
    const size_t win_length = 10;

    MovHistogram<size_t> hist(arena, value_range_min, value_range_max, num_bins,
                              win_length);
    CHECK(hist.is_valid());

    hist.add(5);
    hist.add(15);
    hist.add(25);
    hist.add(35);
    hist.add(45);
    hist.add(55);
    hist.add(65);
    hist.add(75);
    hist.add(85);
    hist.add(95);

    //   0   1   2   3   4   5   6   7   8   9
    // +---+---+---+---+---+---+---+---+---+---+
    // | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 | 1 |
    // +---+---+---+---+---+---+---+---+---+---+

    LONGS_EQUAL(100, hist.mov_quantile(1.0));
    LONGS_EQUAL(40, hist.mov_quantile(0.4));
    LONGS_EQUAL(10, hist.mov_quantile(0.1));

    hist.add(75);
    hist.add(75);
    hist.add(85);

    //   0   1   2   3   4   5   6   7   8   9
    // +---+---+---+---+---+---+---+---+---+---+
    // | 0 | 0 | 0 | 1 | 1 | 1 | 1 | 3 | 2 | 1 |
    // +---+---+---+---+---+---+---+---+---+---+

    LONGS_EQUAL(100, hist.mov_quantile(1.0));
    LONGS_EQUAL(80, hist.mov_quantile(0.7));
    LONGS_EQUAL(70, hist.mov_quantile(0.4));
    LONGS_EQUAL(40, hist.mov_quantile(0.1));
}

} // namespace stat
} // namespace roc
