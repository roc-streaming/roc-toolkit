/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/mov_histogram.h"

namespace roc {
namespace core {

namespace {

enum { NumObjects = 10, EmbeddedCap = 5 };

struct Object {
    static long n_objects;

    size_t value;

    Object(size_t v = 0)
        : value(v) {
        n_objects++;
    }

    Object(const Object& other)
        : value(other.value) {
        n_objects++;
    }

    ~Object() {
        n_objects--;
    }
};

long Object::n_objects = 0;

} // namespace

TEST_GROUP(movhistogram) {
    HeapArena arena;
};

TEST(movhistogram, single_pass) {
    const size_t value_range_min = 0;
    const size_t value_range_max = 100;
    const size_t num_bins = 10;
    const size_t win_length = 10;

    MovHistogram<size_t> hist(arena, value_range_min, value_range_max, num_bins,
                              win_length);
    CHECK(hist.is_valid());

    for (size_t i = 0; i < win_length; i++) {
        hist.add_value(i * num_bins);
    }

    for (size_t i = 0; i < num_bins; ++i) {
        LONGS_EQUAL(1, hist.get_bin_counter(i));
    }
}

TEST(movhistogram, rolling_window) {
    const size_t value_range_min = 0;
    const size_t value_range_max = 100;
    const size_t num_bins = 10;
    const size_t win_length = 5;

    MovHistogram<size_t> hist(arena, value_range_min, value_range_max, num_bins,
                              win_length);
    CHECK(hist.is_valid());

    for (size_t i = 0; i < win_length * 2; i++) {
        hist.add_value(i * (value_range_max / num_bins));
    }

    for (size_t i = 0; i < num_bins; ++i) {
        LONGS_EQUAL(i < win_length ? 0 : 1, hist.get_bin_counter(i));
    }
}

TEST(movhistogram, value_equal_to_value_range_max) {
    const size_t value_range_min = 0;
    const size_t value_range_max = 100;
    const size_t num_bins = 10;
    const size_t win_length = 10;

    MovHistogram<size_t> hist(arena, value_range_min, value_range_max, num_bins,
                              win_length);
    CHECK(hist.is_valid());

    size_t test_value = value_range_max;
    hist.add_value(test_value);

    size_t expected_bin_index = num_bins - 1;
    LONGS_EQUAL(1, hist.get_bin_counter(expected_bin_index));
}

} // namespace core
} // namespace roc
