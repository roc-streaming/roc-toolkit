/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/cpu_traits.h"

namespace roc {
namespace core {

TEST_GROUP(cpu) {};

TEST(cpu, endianess) {
    union {
        uint32_t i;
        char c[4];
    } u = { 0x01020304 };

    const bool is_big = u.c[0] == 0x1;

#if ROC_CPU_ENDIAN == ROC_CPU_BE
    CHECK(is_big);
#else
    CHECK(!is_big);
#endif
}

TEST(cpu, bitness) {
#if ROC_CPU_BITS == 64
    CHECK(sizeof(void*) == 8);
#else
    CHECK(sizeof(void*) < 8);
#endif
}

} // namespace core
} // namespace roc
