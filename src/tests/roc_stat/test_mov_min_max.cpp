/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_harness.h"

#include "roc_core/fast_random.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_stat/mov_min_max.h"

namespace roc {
namespace stat {

TEST_GROUP(mov_min_max) {
    core::HeapArena arena;
};

TEST(mov_min_max, single_pass) {
    const size_t n = 10;
    int64_t x[n] = {};

    MovMinMax<int64_t> comp(arena, n);
    CHECK(comp.is_valid());

    for (size_t i = 0; i < n; i++) {
        x[i] = int64_t((i + 1) * n);
        comp.add(x[i]);
        const int64_t target_min = x[0];
        const int64_t target_max = x[i];
        LONGS_EQUAL(target_min, comp.mov_min());
        LONGS_EQUAL(target_max, comp.mov_max());
    }
}

TEST(mov_min_max, two_passes) {
    const size_t n = 10;
    int64_t x[n] = {};

    MovMinMax<int64_t> comp(arena, n);
    CHECK(comp.is_valid());

    for (size_t i = 0; i < n; i++) {
        x[i] = int64_t((i + 1) * n);
        comp.add(x[i]);
        const int64_t target_min = x[0];
        const int64_t target_max = x[i];
        LONGS_EQUAL(target_min, comp.mov_min());
        LONGS_EQUAL(target_max, comp.mov_max());
    }

    for (size_t i = 0; i < n - 1; i++) {
        int64_t x2 = int64_t((n + i + 1) * n);
        comp.add(x2);
        const int64_t target_min = x[i + 1];
        const int64_t target_max = x2;
        LONGS_EQUAL(target_min, comp.mov_min());
        LONGS_EQUAL(target_max, comp.mov_max());
    }
}

TEST(mov_min_max, one_n_half_pass) {
    const size_t n = 10;

    MovMinMax<int64_t> comp(arena, n);
    CHECK(comp.is_valid());

    const size_t last_i = (n * 10 + n / 2);
    for (size_t i = 0; i < last_i; i++) {
        const int64_t x = int64_t(i * n);
        comp.add(x);
    }

    const int64_t target_min = (last_i - n) * n;
    const int64_t target_max = (last_i - 1) * n;
    LONGS_EQUAL(target_min, comp.mov_min());
    LONGS_EQUAL(target_max, comp.mov_max());
}

TEST(mov_min_max, stress_test) {
    enum { NumIterations = 10, NumElems = 1000, MinWindow = 1, MaxWindow = 100 };

    const int64_t ranges[][2] = {
        { 100000000, 200000000 },
        { -200000000, -100000000 },
        { -100000000, 100000000 },
    };

    for (size_t r = 0; r < ROC_ARRAY_SIZE(ranges); r++) {
        for (size_t i = 0; i < NumIterations; i++) {
            const size_t win_sz = core::fast_random_range(MinWindow, MaxWindow);

            MovMinMax<int64_t> comp(arena, win_sz);
            CHECK(comp.is_valid());

            int64_t elems[NumElems] = {};

            for (size_t n = 0; n < NumElems; n++) {
                elems[n] = ranges[r][0]
                    + (int64_t)core::fast_random_range(
                               0, (uint64_t)(ranges[r][1] - ranges[r][0]));
                comp.add(elems[n]);

                const size_t n_elems = n + 1;

                const size_t cur_win_sz = std::min(win_sz, n_elems);
                const int64_t* cur_win = elems + n_elems - cur_win_sz;

                int64_t target_min = cur_win[0];
                for (size_t n = 1; n < cur_win_sz; ++n) {
                    target_min = std::min(target_min, cur_win[n]);
                }
                int64_t target_max = cur_win[0];
                for (size_t n = 1; n < cur_win_sz; ++n) {
                    target_max = std::max(target_max, cur_win[n]);
                }

                LONGS_EQUAL(target_min, comp.mov_min());
                LONGS_EQUAL(target_max, comp.mov_max());
            }
        }
    }
}

} // namespace stat
} // namespace roc
