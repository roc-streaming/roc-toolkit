/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/memory_limiter.h"

namespace roc {
namespace core {

TEST_GROUP(memory_limiter) {};

TEST(memory_limiter, acquire_release) {
    MemoryLimiter memory_limiter("test", 1024);

    CHECK(memory_limiter.acquire(512) == true);
    CHECK(memory_limiter.num_acquired() == 512);
    CHECK(memory_limiter.acquire(513) == false);
    memory_limiter.release(1);
    CHECK(memory_limiter.acquire(513) == true);
    CHECK(memory_limiter.num_acquired() == 1024);
    memory_limiter.release(1024);
    CHECK(memory_limiter.num_acquired() == 0);
}

} // namespace core
} // namespace roc
