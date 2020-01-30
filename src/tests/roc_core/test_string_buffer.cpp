/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_core/string_buffer.h"

namespace roc {
namespace core {

TEST_GROUP(string_buffer) {
    HeapAllocator allocator;
};

TEST(string_buffer, init) {
    StringBuffer<> sb(allocator);

    CHECK(sb.is_empty());
    UNSIGNED_LONGS_EQUAL(0, sb.len());
    STRCMP_EQUAL("", sb.c_str());

    UNSIGNED_LONGS_EQUAL(1, sb.raw_buf().size());
    UNSIGNED_LONGS_EQUAL(1, sb.raw_buf().max_size());
}

TEST(string_buffer, set_str) {
    StringBuffer<> sb(allocator);

    CHECK(sb.set_str("12345"));

    CHECK(!sb.is_empty());
    UNSIGNED_LONGS_EQUAL(5, sb.len());
    STRCMP_EQUAL("12345", sb.c_str());

    UNSIGNED_LONGS_EQUAL(6, sb.raw_buf().size());
    UNSIGNED_LONGS_EQUAL(6, sb.raw_buf().max_size());
}

TEST(string_buffer, set_buf) {
    StringBuffer<> sb(allocator);

    CHECK(sb.set_buf("12345678", 5));

    CHECK(!sb.is_empty());
    UNSIGNED_LONGS_EQUAL(5, sb.len());
    STRCMP_EQUAL("12345", sb.c_str());

    UNSIGNED_LONGS_EQUAL(6, sb.raw_buf().size());
    UNSIGNED_LONGS_EQUAL(6, sb.raw_buf().max_size());
}

TEST(string_buffer, set_empty) {
    StringBuffer<> sb(allocator);

    CHECK(sb.set_str("12345"));
    CHECK(sb.set_str(""));

    CHECK(sb.is_empty());
    UNSIGNED_LONGS_EQUAL(0, sb.len());
    STRCMP_EQUAL("", sb.c_str());

    UNSIGNED_LONGS_EQUAL(1, sb.raw_buf().size());
    UNSIGNED_LONGS_EQUAL(6, sb.raw_buf().max_size());
}

TEST(string_buffer, clear) {
    StringBuffer<> sb(allocator);

    CHECK(sb.set_str("12345"));
    sb.clear();

    CHECK(sb.is_empty());
    UNSIGNED_LONGS_EQUAL(0, sb.len());
    STRCMP_EQUAL("", sb.c_str());

    UNSIGNED_LONGS_EQUAL(1, sb.raw_buf().size());
    UNSIGNED_LONGS_EQUAL(6, sb.raw_buf().max_size());
}

TEST(string_buffer, overwrite) {
    StringBuffer<> sb(allocator);

    CHECK(sb.set_str("12345678"));
    CHECK(sb.set_str("12345"));

    CHECK(!sb.is_empty());
    UNSIGNED_LONGS_EQUAL(5, sb.len());
    STRCMP_EQUAL("12345", sb.c_str());

    UNSIGNED_LONGS_EQUAL(6, sb.raw_buf().size());
    UNSIGNED_LONGS_EQUAL(9, sb.raw_buf().max_size());
}

TEST(string_buffer, grow) {
    StringBuffer<> sb(allocator);

    CHECK(sb.set_str("12345"));
    CHECK(sb.grow(10));

    CHECK(!sb.is_empty());
    UNSIGNED_LONGS_EQUAL(5, sb.len());
    STRCMP_EQUAL("12345", sb.c_str());

    UNSIGNED_LONGS_EQUAL(6, sb.raw_buf().size());
    UNSIGNED_LONGS_EQUAL(10, sb.raw_buf().max_size());
}

} // namespace core
} // namespace roc
