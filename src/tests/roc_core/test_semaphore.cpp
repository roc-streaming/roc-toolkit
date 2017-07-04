/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/semaphore.h"

namespace roc {
namespace core {

TEST_GROUP(semaphore){};

TEST(semaphore, post_pend) {
    Semaphore sem(0);

    sem.post();
    sem.pend();
}

TEST(semaphore, 2_posts_2_pends) {
    Semaphore sem(0);

    sem.post();
    sem.post();

    sem.pend();
    sem.pend();
}

TEST(semaphore, try_pend) {
    Semaphore sem(0);

    CHECK(!sem.try_pend());

    sem.post();

    CHECK(sem.try_pend());

    CHECK(!sem.try_pend());
}

TEST(semaphore, wait) {
    Semaphore sem(0);

    sem.post();
    sem.wait();

    CHECK(sem.try_pend());
}

TEST(semaphore, non_zero_init_try_pend) {
    enum { Count = 5 };

    Semaphore sem(Count);

    for (size_t n = 0; n < Count; n++) {
        CHECK(sem.try_pend());
    }

    CHECK(!sem.try_pend());
}

TEST(semaphore, non_zero_init_wait_pend) {
    Semaphore sem(1);

    sem.wait();
    sem.pend();
}

} // namespace core
} // namespace roc
