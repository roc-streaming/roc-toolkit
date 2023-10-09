/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/string_buffer.h"

namespace roc {
namespace core {

TEST_GROUP(string_buffer) {
    HeapArena arena;
};

TEST(string_buffer, init) {
    StringBuffer sb(arena);

    CHECK(sb.is_empty());
    UNSIGNED_LONGS_EQUAL(0, sb.len());
    STRCMP_EQUAL("", sb.c_str());
}

TEST(string_buffer, assign) {
    StringBuffer sb(arena);

    CHECK(sb.assign("12345"));

    CHECK(!sb.is_empty());
    UNSIGNED_LONGS_EQUAL(5, sb.len());
    STRCMP_EQUAL("12345", sb.c_str());
}

TEST(string_buffer, assign_range) {
    StringBuffer sb(arena);

    const char* s = "12345678";
    CHECK(sb.assign(s, s + 5));

    CHECK(!sb.is_empty());
    UNSIGNED_LONGS_EQUAL(5, sb.len());
    STRCMP_EQUAL("12345", sb.c_str());
}

TEST(string_buffer, assign_empty) {
    StringBuffer sb(arena);

    CHECK(sb.assign("12345"));
    CHECK(sb.assign(""));

    CHECK(sb.is_empty());
    UNSIGNED_LONGS_EQUAL(0, sb.len());
    STRCMP_EQUAL("", sb.c_str());
}

TEST(string_buffer, assign_overwrite) {
    StringBuffer sb(arena);

    CHECK(sb.assign("12345678"));
    CHECK(sb.assign("12345"));

    CHECK(!sb.is_empty());
    UNSIGNED_LONGS_EQUAL(5, sb.len());
    STRCMP_EQUAL("12345", sb.c_str());
}

TEST(string_buffer, clear) {
    StringBuffer sb(arena);

    CHECK(sb.assign("12345"));
    sb.clear();

    CHECK(sb.is_empty());
    UNSIGNED_LONGS_EQUAL(0, sb.len());
    STRCMP_EQUAL("", sb.c_str());
}

TEST(string_buffer, extend) {
    StringBuffer sb(arena);

    CHECK(sb.assign("1234"));

    UNSIGNED_LONGS_EQUAL(4, sb.len());
    STRCMP_EQUAL("1234", sb.c_str());

    char* ptr = sb.extend(4);
    CHECK(ptr);
    CHECK(!*ptr);
    memcpy(ptr, "5678", 4);

    UNSIGNED_LONGS_EQUAL(8, sb.len());
    STRCMP_EQUAL("12345678", sb.c_str());
}

TEST(string_buffer, grow) {
    StringBuffer sb(arena);

    CHECK(sb.assign("12345"));
    CHECK(sb.grow(10));

    CHECK(!sb.is_empty());
    UNSIGNED_LONGS_EQUAL(5, sb.len());
    STRCMP_EQUAL("12345", sb.c_str());
}

} // namespace core
} // namespace roc
