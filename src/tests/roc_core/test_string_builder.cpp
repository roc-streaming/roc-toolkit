/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/array.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace core {

TEST_GROUP(string_builder) {
    HeapAllocator allocator;
};

TEST(string_builder, init) {
    { // null buffer
        StringBuilder b(NULL, 0);

        CHECK(b.ok());
        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(0, b.actual_size());
    }
    { // zero-size buffer
        char buf[1] = { 'x' };
        StringBuilder b(buf, 0);

        CHECK(!b.ok());
        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(0, b.actual_size());

        CHECK(buf[0] == 'x');
    }
    { // one-byte buffer
        char buf[1] = { 'x' };
        StringBuilder b(buf, 1);

        CHECK(b.ok());
        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(1, b.actual_size());

        CHECK(buf[0] == 0);
    }
    { // zero-size array
        Array<char> array(allocator);
        StringBuilder b(array);

        CHECK(b.ok());
        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(1, b.actual_size());
        UNSIGNED_LONGS_EQUAL(1, array.size());

        CHECK(array[0] == 0);
    }
    { // one-byte array
        Array<char> array(allocator);
        CHECK(array.grow(1));
        array.push_back('x');
        StringBuilder b(array);

        CHECK(b.ok());
        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(1, b.actual_size());
        UNSIGNED_LONGS_EQUAL(1, array.size());

        CHECK(array[0] == 0);
    }
}

TEST(string_builder, set_str) {
    { // copy exact size
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char src[] = "12345678";
        char res[] = "12345678";

        StringBuilder b(dst, Size);

        CHECK(b.set_str_range(src, src + strlen(src)));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(Size, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // copy smaller size
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char src[] = "123456789abcd";
        char res[] = "1234\0xxx";

        StringBuilder b(dst, Size);

        CHECK(b.set_str_range(src, src + 4));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(5, b.needed_size());
        UNSIGNED_LONGS_EQUAL(5, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // copy larger size (truncation)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char src[] = "123456789abcd";
        char res[] = "12345678";

        StringBuilder b(dst, Size);

        CHECK(!b.set_str_range(src, src + strlen(src)));
        CHECK(!b.ok());

        UNSIGNED_LONGS_EQUAL(strlen(src) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // dst is one byte
        enum { Size = 1 };

        char dst[] = "xx";
        char src[] = "12345678";
        char res[] = "\0x";

        StringBuilder b(dst, Size);

        CHECK(!b.set_str_range(src, src + strlen(src)));
        CHECK(!b.ok());

        UNSIGNED_LONGS_EQUAL(strlen(src) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // dst is zero bytes
        enum { Size = 0 };

        char dst[] = "xx";
        char src[] = "12345678";
        char res[] = "xx";

        StringBuilder b(dst, Size);

        CHECK(!b.set_str_range(src, src + strlen(src)));
        CHECK(!b.ok());

        UNSIGNED_LONGS_EQUAL(strlen(src) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // override
        char buf[10];

        StringBuilder b(buf, sizeof(buf));

        CHECK(b.append_str("123"));
        CHECK(b.append_str("456"));
        CHECK(b.ok());

        STRCMP_EQUAL("123456", buf);

        CHECK(b.set_str("abc"));
        CHECK(b.ok());

        STRCMP_EQUAL("abc", buf);
    }
    { // clear error
        char buf[4];

        StringBuilder b(buf, sizeof(buf));

        CHECK(!b.set_str("1235678"));
        CHECK(!b.ok());

        STRCMP_EQUAL("123", buf);

        CHECK(b.set_str("abc"));
        CHECK(b.ok());

        STRCMP_EQUAL("abc", buf);
    }
}

TEST(string_builder, append_str) {
    { // append exact size
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";

        char src1[] = "abcd";
        char res1[] = "abcd\0xxx";

        char src2[] = "1234";
        char res2[] = "abcd1234";

        StringBuilder b(dst, Size);

        CHECK(b.append_str_range(src1, src1 + strlen(src1)));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(strlen(src1) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(strlen(src1) + 1, b.actual_size());

        CHECK(memcmp(dst, res1, sizeof(res1)) == 0);

        CHECK(b.append_str_range(src2, src2 + strlen(src2)));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(Size, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res2, sizeof(res2)) == 0);
    }
    { // append smaller size
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";

        char src1[] = "ab";
        char res1[] = "ab\0xxxxx";

        char src2[] = "1234";
        char res2[] = "ab1234\0x";

        StringBuilder b(dst, Size);

        CHECK(b.append_str_range(src1, src1 + strlen(src1)));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(strlen(src1) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(strlen(src1) + 1, b.actual_size());

        CHECK(memcmp(dst, res1, sizeof(res1)) == 0);

        CHECK(b.append_str_range(src2, src2 + strlen(src2)));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(strlen(src1) + strlen(src2) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(strlen(src1) + strlen(src2) + 1, b.actual_size());

        CHECK(memcmp(dst, res2, sizeof(res2)) == 0);
    }
    { // append larger size (truncation)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";

        char src1[] = "ab";
        char res1[] = "ab\0xxxxx";

        char src2[] = "12345678";
        char res2[] = "ab123456";

        StringBuilder b(dst, Size);

        CHECK(b.append_str_range(src1, src1 + strlen(src1)));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(strlen(src1) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(strlen(src1) + 1, b.actual_size());

        CHECK(memcmp(dst, res1, sizeof(res1)) == 0);

        CHECK(!b.append_str_range(src2, src2 + strlen(src2)));
        CHECK(!b.ok());

        UNSIGNED_LONGS_EQUAL(strlen(src1) + strlen(src2) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res2, sizeof(res2)) == 0);
    }
    { // zero bytes left
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";

        char src1[] = "12345678";
        char res1[] = "12345678";

        char src2[] = "abcd";
        char res2[] = "12345678";

        StringBuilder b(dst, Size);

        CHECK(b.append_str_range(src1, src1 + strlen(src1)));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(Size, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res1, sizeof(res1)) == 0);

        CHECK(!b.append_str_range(src2, src2 + strlen(src2)));
        CHECK(!b.ok());

        UNSIGNED_LONGS_EQUAL(strlen(src1) + strlen(src2) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res2, sizeof(res2)) == 0);
    }
}

TEST(string_builder, append_uint) {
    { // append exact size (10 base)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "----1234";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("----"));
        CHECK(b.ok());

        CHECK(b.append_uint(1234, 10));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(Size, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append exact size (16 base)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "----dead";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("----"));
        CHECK(b.ok());

        CHECK(b.append_uint(0xdead, 16));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(Size, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append smaller size
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "----12\0x";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("----"));
        CHECK(b.ok());

        CHECK(b.append_uint(12, 10));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(7, b.needed_size());
        UNSIGNED_LONGS_EQUAL(7, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append zero
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "----0\0xx";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("----"));
        CHECK(b.ok());

        CHECK(b.append_uint(0, 10));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(6, b.needed_size());
        UNSIGNED_LONGS_EQUAL(6, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append larger size (truncation)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "----1234";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("----"));
        CHECK(b.ok());

        CHECK(!b.append_uint(12345678, 10));
        CHECK(!b.ok());

        UNSIGNED_LONGS_EQUAL(Size + 4, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // zero bytes left
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "--------";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("--------"));
        CHECK(b.ok());

        CHECK(!b.append_uint(1234, 10));
        CHECK(!b.ok());

        UNSIGNED_LONGS_EQUAL(Size + 4, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
}

TEST(string_builder, resize_array) {
    { // set
        Array<char> array(allocator);
        StringBuilder b(array);

        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(1, b.actual_size());

        UNSIGNED_LONGS_EQUAL(1, array.max_size());
        UNSIGNED_LONGS_EQUAL(1, array.size());
        STRCMP_EQUAL("", array.data());

        CHECK(b.set_str("1234"));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(5, b.needed_size());
        UNSIGNED_LONGS_EQUAL(5, b.actual_size());

        UNSIGNED_LONGS_EQUAL(5, array.max_size());
        UNSIGNED_LONGS_EQUAL(5, array.size());
        STRCMP_EQUAL("1234", array.data());

        CHECK(b.set_str("1234abcd"));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(9, b.needed_size());
        UNSIGNED_LONGS_EQUAL(9, b.actual_size());

        UNSIGNED_LONGS_EQUAL(9, array.max_size());
        UNSIGNED_LONGS_EQUAL(9, array.size());
        STRCMP_EQUAL("1234abcd", array.data());
    }
    { // append
        Array<char> array(allocator);
        StringBuilder b(array);

        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(1, b.actual_size());

        UNSIGNED_LONGS_EQUAL(1, array.max_size());
        UNSIGNED_LONGS_EQUAL(1, array.size());
        STRCMP_EQUAL("", array.data());

        CHECK(b.append_str("1234"));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(5, b.needed_size());
        UNSIGNED_LONGS_EQUAL(5, b.actual_size());

        UNSIGNED_LONGS_EQUAL(8, array.max_size());
        UNSIGNED_LONGS_EQUAL(5, array.size());
        STRCMP_EQUAL("1234", array.data());

        CHECK(b.append_str("abcd"));
        CHECK(b.ok());

        UNSIGNED_LONGS_EQUAL(9, b.needed_size());
        UNSIGNED_LONGS_EQUAL(9, b.actual_size());

        UNSIGNED_LONGS_EQUAL(16, array.max_size());
        UNSIGNED_LONGS_EQUAL(9, array.size());
        STRCMP_EQUAL("1234abcd", array.data());
    }
}

} // namespace core
} // namespace roc
