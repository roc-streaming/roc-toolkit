/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/string_list.h"

namespace roc {
namespace core {

TEST_GROUP(string_list) {
    HeapArena arena;
};

TEST(string_list, empty) {
    StringList sl(arena);

    LONGS_EQUAL(0, sl.size());
    CHECK(sl.front() == NULL);
    CHECK(sl.back() == NULL);
}

TEST(string_list, push_back) {
    StringList sl(arena);

    LONGS_EQUAL(0, sl.size());
    CHECK(sl.front() == NULL);
    CHECK(sl.back() == NULL);

    CHECK(sl.push_back("foo"));

    LONGS_EQUAL(1, sl.size());
    STRCMP_EQUAL("foo", sl.front());
    STRCMP_EQUAL("foo", sl.back());

    CHECK(sl.push_back("barbaz"));

    LONGS_EQUAL(2, sl.size());
    STRCMP_EQUAL("foo", sl.front());
    STRCMP_EQUAL("barbaz", sl.back());
}

TEST(string_list, pop_back) {
    StringList sl(arena);

    LONGS_EQUAL(0, sl.size());
    CHECK(sl.push_back("foo"));
    CHECK(sl.pop_back());
    LONGS_EQUAL(0, sl.size());

    CHECK(sl.push_back("foo"));
    CHECK(sl.push_back("barbaz"));
    CHECK(sl.pop_back());
    LONGS_EQUAL(1, sl.size());
    STRCMP_EQUAL("foo", sl.front());
    STRCMP_EQUAL("foo", sl.back());

    CHECK(sl.push_back("foobarbaz"));
    CHECK(sl.push_back("baz"));
    CHECK(sl.pop_back());
    LONGS_EQUAL(2, sl.size());
    STRCMP_EQUAL("foo", sl.front());
    STRCMP_EQUAL("foobarbaz", sl.back());
}

TEST(string_list, push_back_range) {
    StringList sl(arena);

    LONGS_EQUAL(0, sl.size());
    CHECK(sl.front() == NULL);

    const char* str = "foobarbaz";

    CHECK(sl.push_back(str, str + 3));

    LONGS_EQUAL(1, sl.size());
    STRCMP_EQUAL("foo", sl.front());
    STRCMP_EQUAL("foo", sl.back());

    CHECK(sl.push_back(str + 3, str + 6));

    LONGS_EQUAL(2, sl.size());
    STRCMP_EQUAL("foo", sl.front());
    STRCMP_EQUAL("bar", sl.back());
}

TEST(string_list, nextof) {
    StringList sl(arena);

    CHECK(sl.push_back("foo"));
    CHECK(sl.push_back("barbaz"));
    CHECK(sl.push_back("foobarbaz"));

    LONGS_EQUAL(3, sl.size());

    const char* str = NULL;

    str = sl.front();
    STRCMP_EQUAL("foo", str);

    str = sl.nextof(str);
    STRCMP_EQUAL("barbaz", str);

    str = sl.nextof(str);
    STRCMP_EQUAL("foobarbaz", str);

    str = sl.nextof(str);
    CHECK(str == NULL);
}

TEST(string_list, prevof) {
    StringList sl(arena);

    CHECK(sl.push_back("foo"));
    CHECK(sl.push_back("barbaz"));
    CHECK(sl.push_back("foobarbaz"));

    LONGS_EQUAL(3, sl.size());

    const char* str = NULL;

    str = sl.back();
    STRCMP_EQUAL("foobarbaz", str);

    str = sl.prevof(str);
    STRCMP_EQUAL("barbaz", str);

    str = sl.prevof(str);
    STRCMP_EQUAL("foo", str);

    str = sl.prevof(str);
    CHECK(str == NULL);
}

TEST(string_list, copy) {
    StringList sl(arena);

    const char* src = "foo";

    CHECK(sl.push_back(src));
    CHECK(sl.push_back(src));

    LONGS_EQUAL(2, sl.size());

    const char* elem1 = sl.front();
    const char* elem2 = sl.nextof(elem1);

    STRCMP_EQUAL("foo", elem1);
    STRCMP_EQUAL("foo", elem2);

    CHECK(elem1 != src);
    CHECK(elem2 != src);
    CHECK(elem1 != elem2);
}

TEST(string_list, empty_strings) {
    StringList sl(arena);

    CHECK(sl.push_back(""));
    CHECK(sl.push_back(""));
    CHECK(sl.push_back(""));

    LONGS_EQUAL(3, sl.size());

    const char* str = NULL;

    str = sl.front();
    STRCMP_EQUAL("", str);

    str = sl.nextof(str);
    STRCMP_EQUAL("", str);

    str = sl.nextof(str);
    STRCMP_EQUAL("", str);

    str = sl.nextof(str);
    CHECK(str == NULL);
}

TEST(string_list, find) {
    StringList sl(arena);

    CHECK(sl.push_back("foo"));
    CHECK(sl.push_back("barbaz"));
    CHECK(sl.push_back("foobarbaz"));

    LONGS_EQUAL(3, sl.size());

    CHECK(sl.find("barbaz"));
    CHECK(!sl.find("qux"));

    LONGS_EQUAL(3, sl.size());

    const char* str = NULL;

    str = sl.front();
    STRCMP_EQUAL("foo", str);

    str = sl.nextof(str);
    STRCMP_EQUAL("barbaz", str);

    str = sl.nextof(str);
    STRCMP_EQUAL("foobarbaz", str);

    str = sl.nextof(str);
    CHECK(str == NULL);
}

TEST(string_list, find_range) {
    StringList sl(arena);

    CHECK(sl.push_back("foo"));
    CHECK(sl.push_back("bar"));

    LONGS_EQUAL(2, sl.size());

    const char* str = "foobarbaz";

    STRCMP_EQUAL("foo", sl.find(str, str + 3));
    STRCMP_EQUAL("bar", sl.find(str + 3, str + 6));

    CHECK(sl.find(str + 6, str + 9) == NULL);
}

TEST(string_list, clear) {
    StringList sl(arena);

    CHECK(sl.push_back("foo"));

    LONGS_EQUAL(1, sl.size());
    STRCMP_EQUAL("foo", sl.front());

    sl.clear();

    LONGS_EQUAL(0, sl.size());
    CHECK(sl.front() == NULL);

    CHECK(sl.push_back("barbaz"));

    LONGS_EQUAL(1, sl.size());
    STRCMP_EQUAL("barbaz", sl.front());
}

TEST(string_list, exponential_growth) {
    StringList sl(arena);

    const char* prev_front = sl.front();
    int num_reallocs = 0;

    int expected_reallocs[] = {
        1, 1,                            //
        2, 2, 2,                         //
        3, 3, 3, 3, 3,                   //
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, //
        5, 5, 5, 5, 5                    //
    };

    for (size_t n = 0; n < ROC_ARRAY_SIZE(expected_reallocs); n++) {
        CHECK(sl.push_back("123456789abcd,123456789abcd"));

        if (prev_front != sl.front()) {
            num_reallocs++;
            prev_front = sl.front();
        }

        LONGS_EQUAL(n + 1, sl.size());
        LONGS_EQUAL(expected_reallocs[n], num_reallocs);
    }
}

} // namespace core
} // namespace roc
