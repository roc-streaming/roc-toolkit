/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/cpu_instructions.h"
#include "roc_core/cpu_traits.h"

namespace roc {
namespace core {

TEST_GROUP(cpu) {};

TEST(cpu, family) {
#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_X86_64
    CHECK(ROC_CPU_ENDIAN == ROC_CPU_ENDIAN_LE);
    CHECK(ROC_CPU_BITS == 64);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_X86
    CHECK(ROC_CPU_ENDIAN == ROC_CPU_ENDIAN_LE);
    CHECK(ROC_CPU_BITS == 32);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_PPC64
    CHECK(ROC_CPU_BITS == 64);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_PPC
    CHECK(ROC_CPU_BITS == 32);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_S390X
    CHECK(ROC_CPU_ENDIAN == ROC_CPU_ENDIAN_BE);
    CHECK(ROC_CPU_BITS == 64);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_S390
    CHECK(ROC_CPU_ENDIAN == ROC_CPU_ENDIAN_BE);
    CHECK(ROC_CPU_BITS == 32);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_LOONGARCH64
    CHECK(ROC_CPU_ENDIAN == ROC_CPU_ENDIAN_LE);
    CHECK(ROC_CPU_BITS == 64);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_LOONGARCH32
    CHECK(ROC_CPU_ENDIAN == ROC_CPU_ENDIAN_LE);
    CHECK(ROC_CPU_BITS == 32);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_AARCH64
    CHECK(ROC_CPU_BITS == 64);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_ARM
    CHECK(ROC_CPU_BITS == 32);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_MIPS64
    CHECK(ROC_CPU_BITS == 64);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_MIPS
    CHECK(ROC_CPU_BITS == 32);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_RISCV64
    CHECK(ROC_CPU_BITS == 64);
#endif

#if ROC_CPU_FAMILY == ROC_CPU_FAMILY_RISCV32
    CHECK(ROC_CPU_BITS == 32);
#endif
}

TEST(cpu, endian) {
    union {
        uint32_t i;
        char c[4];
    } u = { 0x01020304 };

    const bool is_be = u.c[0] == 0x1;

#if ROC_CPU_ENDIAN == ROC_CPU_ENDIAN_BE
    CHECK(is_be);
#else
    CHECK(ROC_CPU_ENDIAN == ROC_CPU_ENDIAN_LE);
    CHECK(!is_be);
#endif
}

TEST(cpu, bits) {
#if ROC_CPU_BITS == 64
    CHECK(sizeof(void*) == 8);
#else
    CHECK(ROC_CPU_BITS == 32);
    CHECK(sizeof(void*) == 4);
#endif
}

TEST(cpu, relax) {
    cpu_relax();
}

} // namespace core
} // namespace roc
