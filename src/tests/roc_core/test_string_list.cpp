/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_core/helpers.h"
#include "roc_core/string_list.h"

namespace roc {
namespace core {

TEST_GROUP(string_list) {
    HeapAllocator allocator;
};

TEST(string_list, empty) {
    StringList sl(allocator);

    LONGS_EQUAL(0, sl.size());
    CHECK(sl.front() == NULL);
}

TEST(string_list, push_back) {
    StringList sl(allocator);

    LONGS_EQUAL(0, sl.size());
    CHECK(sl.front() == NULL);

    CHECK(sl.push_back("foo"));

    LONGS_EQUAL(1, sl.size());
    STRCMP_EQUAL("foo", sl.front());

    CHECK(sl.push_back("bar"));

    LONGS_EQUAL(2, sl.size());
    STRCMP_EQUAL("foo", sl.front());
}

TEST(string_list, nextof) {
    StringList sl(allocator);

    CHECK(sl.push_back("foo"));
    CHECK(sl.push_back("bar"));
    CHECK(sl.push_back("baz"));

    LONGS_EQUAL(3, sl.size());

    const char* str = NULL;

    str = sl.front();
    STRCMP_EQUAL("foo", str);

    str = sl.nextof(str);
    STRCMP_EQUAL("bar", str);

    str = sl.nextof(str);
    STRCMP_EQUAL("baz", str);

    str = sl.nextof(str);
    CHECK(str == NULL);
}

TEST(string_list, copy) {
    StringList sl(allocator);

    const char* src = "foo";

    sl.push_back(src);
    sl.push_back(src);

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
    StringList sl(allocator);

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

TEST(string_list, uniq) {
    StringList sl(allocator);

    CHECK(sl.push_back("foo"));
    CHECK(sl.push_back("bar"));
    CHECK(sl.push_back("baz"));

    LONGS_EQUAL(3, sl.size());

    CHECK(sl.push_back_uniq("bar"));
    CHECK(sl.push_back_uniq("qux"));

    LONGS_EQUAL(4, sl.size());

    const char* str = NULL;

    str = sl.front();
    STRCMP_EQUAL("foo", str);

    str = sl.nextof(str);
    STRCMP_EQUAL("bar", str);

    str = sl.nextof(str);
    STRCMP_EQUAL("baz", str);

    str = sl.nextof(str);
    STRCMP_EQUAL("qux", str);

    str = sl.nextof(str);
    CHECK(str == NULL);
}

TEST(string_list, clear) {
    StringList sl(allocator);

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
    StringList sl(allocator);

    const char* prev_front = sl.front();
    int num_reallocs = 0;

    int expected_reallocs[] = {
        1, 1, 1, 1,                                    //
        2, 2, 2, 2,                                    //
        3, 3, 3, 3, 3, 3, 3, 3,                        //
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 //
    };

    for (size_t n = 0; n < ROC_ARRAY_SIZE(expected_reallocs); n++) {
        CHECK(sl.push_back("123456789abcdef,123456789abcdef"));

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
