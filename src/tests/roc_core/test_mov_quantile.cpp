/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/mov_quantile.h"

namespace roc {
namespace core {

TEST_GROUP(mov_quantile) {
    HeapArena arena;
};

TEST(mov_quantile, testing_minimum) {
    const size_t n = 9;
    MovQuantile<int64_t> quant(arena, n, 0.0);
    CHECK(quant.is_valid());
    quant.add(14);
    quant.add(28);
    quant.add(11);
    quant.add(12);
    quant.add(18);
    quant.add(15);
    quant.add(25);
    LONGS_EQUAL((int64_t)11, quant.mov_quantile()); // test window incomplete
    quant.add(32);
    quant.add(14);
    quant.add(19);
    quant.add(16);
    quant.add(35);
    LONGS_EQUAL((int64_t)12, quant.mov_quantile()); // test window complete
}

TEST(mov_quantile, testing_lower_side) {
    const size_t n = 12;
    MovQuantile<int64_t> quant(arena, n, 0.34);
    CHECK(quant.is_valid());
    quant.add(10);
    quant.add(12);
    quant.add(25);
    quant.add(22);
    quant.add(18);
    quant.add(6);
    quant.add(24);
    LONGS_EQUAL((int64_t)12, quant.mov_quantile()); // test window incomplete
    quant.add(22);
    quant.add(35);
    quant.add(42);
    quant.add(31);
    quant.add(39);
    quant.add(27);
    quant.add(4);
    quant.add(45);
    quant.add(49);
    quant.add(37);
    int64_t x1 = quant.mov_quantile(); // test complete window insertion
    LONGS_EQUAL((int64_t)24, x1);
}

TEST(mov_quantile, testing_median) {
    const size_t n = 10;
    MovQuantile<int64_t> quant(arena, n, 0.50);
    CHECK(quant.is_valid());
    quant.add(18);
    quant.add(12);
    quant.add(55);
    quant.add(72);
    quant.add(25);
    quant.add(6);
    quant.add(37);
    LONGS_EQUAL((int64_t)25, quant.mov_quantile()); // test window incomplete
    quant.add(23);
    quant.add(48);
    quant.add(100);
    quant.add(62);
    quant.add(57);
    quant.add(92);
    quant.add(1);
    quant.add(72);
    quant.add(83);
    quant.add(37);
    LONGS_EQUAL((int64_t)57, quant.mov_quantile()); // test complete window
}

TEST(mov_quantile, testing_upper_side) {
    const size_t n = 11;
    MovQuantile<int64_t> quant(arena, n, 0.78);
    CHECK(quant.is_valid());
    quant.add(18);
    quant.add(18);
    quant.add(22);
    quant.add(14);
    quant.add(39);
    quant.add(52);
    quant.add(14);
    quant.add(46);
    LONGS_EQUAL((int64_t)39, quant.mov_quantile()); // test incomplete window
    quant.add(14);
    quant.add(14);
    quant.add(100);
    quant.add(32);
    quant.add(83);
    LONGS_EQUAL((int64_t)46, quant.mov_quantile()); // test complete window
}

TEST(mov_quantile, test_maximum) {
    const size_t n = 7;
    MovQuantile<int64_t> quant(arena, n, 1);
    CHECK(quant.is_valid());
    quant.add(21);
    quant.add(14);
    quant.add(38);
    quant.add(72);
    quant.add(63);
    LONGS_EQUAL((int64_t)72, quant.mov_quantile()); // test incomplete window
    quant.add(35);
    quant.add(76);
    quant.add(42);
    quant.add(13);
    quant.add(15);
    quant.add(11);
    quant.add(102);
    quant.add(56);
    quant.add(20);
    LONGS_EQUAL((int64_t)102, quant.mov_quantile()); // test complete window
}

} // namespace core
} // namespace roc
