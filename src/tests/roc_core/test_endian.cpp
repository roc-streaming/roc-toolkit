/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/endian.h"
#include "roc_core/endian_ops.h"

namespace roc {
namespace core {

TEST_GROUP(endian) {};

TEST(endian, integers) {
    CHECK(0x11 == EndianOps::swap_endian((uint8_t)0x11));
    CHECK(0x11 == EndianOps::swap_endian((int8_t)0x11));

    CHECK(0x2211 == EndianOps::swap_endian((uint16_t)0x1122));
    CHECK(0x2211 == EndianOps::swap_endian((int16_t)0x1122));

    CHECK(0x44332211 == EndianOps::swap_endian((uint32_t)0x11223344));
    CHECK(0x44332211 == EndianOps::swap_endian((int32_t)0x11223344));

    CHECK(0x8877665544332211 == EndianOps::swap_endian((uint64_t)0x1122334455667788));
    CHECK((int64_t)0x8877665544332211
          == EndianOps::swap_endian((int64_t)0x1122334455667788));
}

TEST(endian, floats) {
    DOUBLES_EQUAL(1.2345, EndianOps::swap_endian(EndianOps::swap_endian(1.2345f)), 1e-6f);
    DOUBLES_EQUAL(1.2345, EndianOps::swap_endian(EndianOps::swap_endian(1.2345f)), 1e-6f);

    DOUBLES_EQUAL(1.2345, EndianOps::swap_endian(EndianOps::swap_endian(1.2345)), 1e-6f);
    DOUBLES_EQUAL(1.2345, EndianOps::swap_endian(EndianOps::swap_endian(1.2345)), 1e-6f);
}

TEST(endian, big_little) {
#if ROC_CPU_ENDIAN == ROC_CPU_ENDIAN_BE
    CHECK(0x1122 == EndianOps::swap_native_be((uint16_t)0x1122));
    CHECK(0x2211 == EndianOps::swap_native_le((uint16_t)0x1122));
#else
    CHECK(0x2211 == EndianOps::swap_native_be((uint16_t)0x1122));
    CHECK(0x1122 == EndianOps::swap_native_le((uint16_t)0x1122));
#endif
}

TEST(endian, ntoh) {
#if ROC_CPU_ENDIAN == ROC_CPU_ENDIAN_BE
    CHECK(0x1122 == ntoh16u((uint16_t)0x1122));
    CHECK(0x1122 == ntoh16s((int16_t)0x1122));

    CHECK(0x11223344 == ntoh32u((uint32_t)0x11223344));
    CHECK(0x11223344 == ntoh32s((int32_t)0x11223344));

    CHECK(0x1122334455667788 == ntoh64u((uint64_t)0x1122334455667788));
    CHECK((int64_t)0x1122334455667788 == ntoh64s((int64_t)0x1122334455667788));
#else
    CHECK(0x2211 == ntoh16u((uint16_t)0x1122));
    CHECK(0x2211 == ntoh16s((int16_t)0x1122));

    CHECK(0x44332211 == ntoh32u((uint32_t)0x11223344));
    CHECK(0x44332211 == ntoh32s((int32_t)0x11223344));

    CHECK(0x8877665544332211 == ntoh64u((uint64_t)0x1122334455667788));
    CHECK((int64_t)0x8877665544332211 == ntoh64s((int64_t)0x1122334455667788));
#endif
}

TEST(endian, hton) {
#if ROC_CPU_ENDIAN == ROC_CPU_ENDIAN_BE
    CHECK(0x1122 == hton16u((uint16_t)0x1122));
    CHECK(0x1122 == hton16s((int16_t)0x1122));

    CHECK(0x11223344 == hton32u((uint32_t)0x11223344));
    CHECK(0x11223344 == hton32s((int32_t)0x11223344));

    CHECK(0x1122334455667788 == hton64u((uint64_t)0x1122334455667788));
    CHECK((int64_t)0x1122334455667788 == hton64s((int64_t)0x1122334455667788));
#else
    CHECK(0x2211 == hton16u((uint16_t)0x1122));
    CHECK(0x2211 == hton16s((int16_t)0x1122));

    CHECK(0x44332211 == hton32u((uint32_t)0x11223344));
    CHECK(0x44332211 == hton32s((int32_t)0x11223344));

    CHECK(0x8877665544332211 == hton64u((uint64_t)0x1122334455667788));
    CHECK((int64_t)0x8877665544332211 == hton64s((int64_t)0x1122334455667788));
#endif
}

} // namespace core
} // namespace roc
