/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/seqlock.h"

namespace roc {
namespace core {

TEST_GROUP(seqlock) {};

TEST(seqlock, int32) {
    Seqlock<int32_t> sl(345);
    LONGS_EQUAL(345, sl.wait_load());

    sl.exclusive_store(123);
    LONGS_EQUAL(123, sl.wait_load());

    int32_t v1 = 0;
    CHECK(sl.try_load(v1));
    LONGS_EQUAL(123, v1);

    CHECK(sl.try_store(456));
    LONGS_EQUAL(456, sl.wait_load());

    int32_t v2 = 0;
    CHECK(sl.try_load(v2));
    LONGS_EQUAL(456, v2);
}

TEST(seqlock, int64) {
    Seqlock<int64_t> sl(345);
    LONGS_EQUAL(345, sl.wait_load());

    sl.exclusive_store(123);
    LONGS_EQUAL(123, sl.wait_load());

    int64_t v1 = 0;
    CHECK(sl.try_load(v1));
    LONGS_EQUAL(123, v1);

    CHECK(sl.try_store(456));
    LONGS_EQUAL(456, sl.wait_load());

    int64_t v2 = 0;
    CHECK(sl.try_load(v2));
    LONGS_EQUAL(456, v2);
}

TEST(seqlock, 64bit_atomics) {
    if (ROC_CPU_64BIT) {
        // we have 8-byte pointers
        CHECK(sizeof(void*) == 8);

        // Seqlock<uint64_t> and Seqlock<int64_t> are optimized to atomics
        CHECK(sizeof(Seqlock<uint64_t>) == sizeof(AtomicSeqlock<uint64_t>));
        CHECK(sizeof(Seqlock<int64_t>) == sizeof(AtomicSeqlock<int64_t>));
    } else {
        // we have smaller pointers
        CHECK(sizeof(void*) < 8);

        // Seqlock<uint64_t> and Seqlock<int64_t> are not optimized to atomics
        CHECK(sizeof(Seqlock<uint64_t>) != sizeof(AtomicSeqlock<uint64_t>));
        CHECK(sizeof(Seqlock<int64_t>) != sizeof(AtomicSeqlock<int64_t>));
    }
}

} // namespace core
} // namespace roc
