/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/fast_random.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_stat/mov_aggregate.h"

namespace roc {
namespace stat {

TEST_GROUP(mov_aggregate) {
    core::HeapArena arena;
};

TEST(mov_aggregate, single_pass) {
    const size_t n = 10;
    int64_t x[n];
    MovAggregate<int64_t> agg(arena, n);
    for (size_t i = 0; i < n; i++) {
        x[i] = int64_t(i * n);
        agg.add(x[i]);
        double target_avg = 0;
        for (size_t j = 0; j <= i; ++j) {
            target_avg += double(x[j]);
        }
        target_avg /= double(i + 1);
        double target_var = 0;
        for (size_t j = 0; j <= i; ++j) {
            target_var += (double)(x[j] - target_avg) * (double)(x[j] - target_avg);
        }
        target_var = target_var / double(i + 1);
        double target_std = sqrt(target_var);
        LONGS_EQUAL((int64_t)round(target_avg), agg.mov_avg());
        LONGS_EQUAL((int64_t)round(target_var), agg.mov_var());
        LONGS_EQUAL((int64_t)round(target_std), agg.mov_std());
    }
}

TEST(mov_aggregate, one_n_half_pass) {
    const size_t n = 10;
    MovAggregate<int64_t> agg(arena, n);
    for (size_t i = 0; i < (n * 10 + n / 2); i++) {
        const int64_t x = (int64_t)pow(-1., (double)i);
        agg.add(x);
    }

    LONGS_EQUAL(0, agg.mov_avg());
    LONGS_EQUAL(1, agg.mov_var());

    double target_avg = double(n - 1) * n / 2;
    double target_var = 0;
    for (size_t i = 0; i < n; i++) {
        const int64_t x = int64_t(i * n);
        agg.add(x);
        target_var += (x - target_avg) * (x - target_avg);
    }
    target_var /= double(n);
    double target_std = sqrt(target_var);

    LONGS_EQUAL((int64_t)round(target_avg), agg.mov_avg());
    LONGS_EQUAL((int64_t)round(target_var), agg.mov_var());
    LONGS_EQUAL((int64_t)round(target_std), agg.mov_std());
}

TEST(mov_aggregate, stress_test) {
    enum { NumIterations = 10, NumElems = 1000, MinWindow = 1, MaxWindow = 100 };

    const int64_t ranges[][2] = {
        { 100000000, 200000000 },
        { -200000000, -100000000 },
        { -100000000, 100000000 },
    };

    for (size_t r = 0; r < ROC_ARRAY_SIZE(ranges); r++) {
        for (size_t i = 0; i < NumIterations; i++) {
            const size_t win_sz = core::fast_random_range(MinWindow, MaxWindow);

            MovAggregate<int64_t> agg(arena, win_sz);
            CHECK(agg.is_valid());

            int64_t elems[NumElems] = {};

            for (size_t n = 0; n < NumElems; n++) {
                elems[n] = ranges[r][0]
                    + (int64_t)core::fast_random_range(
                               0, (uint64_t)(ranges[r][1] - ranges[r][0]));
                agg.add(elems[n]);

                const size_t n_elems = n + 1;

                const size_t cur_win_sz = std::min(win_sz, n_elems);
                const int64_t* cur_win = elems + n_elems - cur_win_sz;

                double target_avg = 0;
                for (size_t n = 0; n < cur_win_sz; ++n) {
                    target_avg += double(cur_win[n]) / cur_win_sz;
                }
                double target_var = 0;
                for (size_t n = 0; n < cur_win_sz; ++n) {
                    target_var += pow(cur_win[n] - target_avg, 2) / cur_win_sz;
                }
                double target_std = sqrt(target_var);

                DOUBLES_EQUAL(target_avg, agg.mov_avg(), 1);
                DOUBLES_EQUAL(target_var, agg.mov_var(), 100);
                DOUBLES_EQUAL(target_std, agg.mov_std(), 100);
            }
        }
    }
}

} // namespace stat
} // namespace roc
