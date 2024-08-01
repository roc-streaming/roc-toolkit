/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
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
    double target_avg = 0;
    double target_var = 0;
    for (size_t i = 0; i < n; i++) {
        x[i] = int64_t(i * n);
        agg.add(x[i]);
        target_avg = target_var = 0;
        for (size_t j = 0; j <= i; ++j) {
            target_avg += double(x[j]);
        }
        target_avg /= double(i + 1);
        for (size_t j = 0; j <= i; ++j) {
            target_var += (double)(x[j] - target_avg) * (double)(x[j] - target_avg);
        }
        target_var = sqrt(target_var / double(i + 1));
        LONGS_EQUAL((int64_t)target_avg, agg.mov_avg());
        LONGS_EQUAL((int64_t)target_var, agg.mov_var());
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

    const int64_t target_avg = (n - 1) * n / 2;
    int64_t target_var = 0;
    for (size_t i = 0; i < n; i++) {
        const int64_t x = int64_t(i * n);
        agg.add(x);
        target_var += (x - target_avg) * (x - target_avg);
    }
    target_var = (int64_t)sqrt((double)target_var / n);

    LONGS_EQUAL(target_avg, agg.mov_avg());
    LONGS_EQUAL(target_var, agg.mov_var());
}

TEST(mov_aggregate, one_n_half_extend) {
    const size_t n = 10;
    MovAggregate<int64_t> agg(arena, n);
    const int64_t target_avg = n;
    int64_t target_var = 0;
    size_t i = 0;
    for (; i < n / 2; i++) {
        const int64_t x = (int64_t)i + 1;
        agg.add(x);
    }
    for (; i < (n + n / 2); i++) {
        const int64_t x = (int64_t)i + 1;
        agg.add(x);
        target_var += (x - target_avg) * (x - target_avg);
    }
    target_var = (int64_t)sqrt((double)target_var / n);

    LONGS_EQUAL(target_avg, agg.mov_avg());
    LONGS_EQUAL(target_var, agg.mov_var());

    CHECK(agg.extend_win(n * 10));

    LONGS_EQUAL((int64_t)ceil(n * 1.25), agg.mov_avg()); // [n; n + n/2]
    LONGS_EQUAL(target_var / 2, agg.mov_var());
}

} // namespace stat
} // namespace roc
