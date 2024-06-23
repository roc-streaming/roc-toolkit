/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/array.h"
#include "roc_core/heap_arena.h"
#include "roc_core/string_buffer.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace core {

TEST_GROUP(string_builder) {
    HeapArena arena;
};

TEST(string_builder, init) {
    { // null zero-size buffer
        StringBuilder b(NULL, 0);

        CHECK(b.is_ok());
        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(0, b.actual_size());
    }
    { // null one-byte buffer
        StringBuilder b(NULL, 1);

        CHECK(b.is_ok());
        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(0, b.actual_size());
    }
    { // zero-size buffer
        char buf[1] = { 'x' };
        StringBuilder b(buf, 0);

        CHECK(!b.is_ok());
        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(0, b.actual_size());

        CHECK(buf[0] == 'x');
    }
    { // one-byte buffer
        char buf[1] = { 'x' };
        StringBuilder b(buf, 1);

        CHECK(b.is_ok());
        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(1, b.actual_size());

        CHECK(buf[0] == 0);
    }
    { // zero-size StringBuffer
        StringBuffer buf(arena);
        StringBuilder b(buf);

        CHECK(b.is_ok());
        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(1, b.actual_size());
        UNSIGNED_LONGS_EQUAL(0, buf.len());

        CHECK(*buf.c_str() == '\0');
    }
    { // one-byte StringBuffer
        StringBuffer buf(arena);
        CHECK(buf.assign("x"));

        StringBuilder b(buf);

        CHECK(b.is_ok());
        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(1, b.actual_size());
        UNSIGNED_LONGS_EQUAL(0, buf.len());

        CHECK(*buf.c_str() == '\0');
    }
}

TEST(string_builder, rewrite) {
    { // copy exact size
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char src[] = "12345678\0";
        char res[] = "12345678";

        StringBuilder b(dst, Size);

        CHECK(b.rewrite(src));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(Size, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // copy smaller size
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char src[] = "1234\0";
        char res[] = "1234\0xxx";

        StringBuilder b(dst, Size);

        CHECK(b.rewrite(src));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(5, b.needed_size());
        UNSIGNED_LONGS_EQUAL(5, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // copy larger size (truncation)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char src[] = "123456789abcd\0";
        char res[] = "12345678";

        StringBuilder b(dst, Size);

        CHECK(!b.rewrite(src));
        CHECK(!b.is_ok());

        UNSIGNED_LONGS_EQUAL(strlen(src) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // dst is one byte
        enum { Size = 1 };

        char dst[] = "xx";
        char src[] = "12345678\0";
        char res[] = "\0x";

        StringBuilder b(dst, Size);

        CHECK(!b.rewrite(src));
        CHECK(!b.is_ok());

        UNSIGNED_LONGS_EQUAL(strlen(src) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // dst is zero bytes
        enum { Size = 0 };

        char dst[] = "xx";
        char src[] = "12345678\0";
        char res[] = "xx";

        StringBuilder b(dst, Size);

        CHECK(!b.rewrite(src));
        CHECK(!b.is_ok());

        UNSIGNED_LONGS_EQUAL(strlen(src) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // override
        char buf[10];

        StringBuilder b(buf, sizeof(buf));

        CHECK(b.append_str("123"));
        CHECK(b.append_str("456"));
        CHECK(b.is_ok());

        STRCMP_EQUAL("123456", buf);

        CHECK(b.rewrite("abc"));
        CHECK(b.is_ok());

        STRCMP_EQUAL("abc", buf);
    }
    { // clear error
        char buf[4];

        StringBuilder b(buf, sizeof(buf));

        CHECK(!b.rewrite("1235678"));
        CHECK(!b.is_ok());

        STRCMP_EQUAL("123", buf);

        CHECK(b.rewrite("abc"));
        CHECK(b.is_ok());

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

        CHECK(b.append_range(src1, src1 + strlen(src1)));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(strlen(src1) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(strlen(src1) + 1, b.actual_size());

        CHECK(memcmp(dst, res1, sizeof(res1)) == 0);

        CHECK(b.append_range(src2, src2 + strlen(src2)));
        CHECK(b.is_ok());

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

        CHECK(b.append_range(src1, src1 + strlen(src1)));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(strlen(src1) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(strlen(src1) + 1, b.actual_size());

        CHECK(memcmp(dst, res1, sizeof(res1)) == 0);

        CHECK(b.append_range(src2, src2 + strlen(src2)));
        CHECK(b.is_ok());

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

        CHECK(b.append_range(src1, src1 + strlen(src1)));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(strlen(src1) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(strlen(src1) + 1, b.actual_size());

        CHECK(memcmp(dst, res1, sizeof(res1)) == 0);

        CHECK(!b.append_range(src2, src2 + strlen(src2)));
        CHECK(!b.is_ok());

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

        CHECK(b.append_range(src1, src1 + strlen(src1)));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(Size, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res1, sizeof(res1)) == 0);

        CHECK(!b.append_range(src2, src2 + strlen(src2)));
        CHECK(!b.is_ok());

        UNSIGNED_LONGS_EQUAL(strlen(src1) + strlen(src2) + 1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res2, sizeof(res2)) == 0);
    }
}

TEST(string_builder, append_sint) {
    { // append exact size (10 base)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "...-1234";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("..."));
        CHECK(b.is_ok());

        CHECK(b.append_sint(-1234, 10));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(Size, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append exact size (16 base)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "...-DEAD";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("..."));
        CHECK(b.is_ok());

        CHECK(b.append_sint(-0xdead, 16));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(Size, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append exact size (positive)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "....1234";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("...."));
        CHECK(b.is_ok());

        CHECK(b.append_sint(1234, 10));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(Size, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append smaller size
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "...-12\0x";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("..."));
        CHECK(b.is_ok());

        CHECK(b.append_sint(-12, 10));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(7, b.needed_size());
        UNSIGNED_LONGS_EQUAL(7, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append larger size (truncation)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "...-1234";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("..."));
        CHECK(b.is_ok());

        CHECK(!b.append_sint(-12345678, 10));
        CHECK(!b.is_ok());

        UNSIGNED_LONGS_EQUAL(Size + 4, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // zero bytes left
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "........";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("........"));
        CHECK(b.is_ok());

        CHECK(!b.append_sint(-1234, 10));
        CHECK(!b.is_ok());

        UNSIGNED_LONGS_EQUAL(Size + 5, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
}

TEST(string_builder, append_uint) {
    { // append exact size (10 base)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "....1234";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("...."));
        CHECK(b.is_ok());

        CHECK(b.append_uint(1234, 10));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(Size, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append exact size (16 base)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "....DEAD";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("...."));
        CHECK(b.is_ok());

        CHECK(b.append_uint(0xdead, 16));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(Size, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append smaller size
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "....12\0x";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("...."));
        CHECK(b.is_ok());

        CHECK(b.append_uint(12, 10));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(7, b.needed_size());
        UNSIGNED_LONGS_EQUAL(7, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append zero
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "....0\0xx";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("...."));
        CHECK(b.is_ok());

        CHECK(b.append_uint(0, 10));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(6, b.needed_size());
        UNSIGNED_LONGS_EQUAL(6, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append larger size (truncation)
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "....1234";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("...."));
        CHECK(b.is_ok());

        CHECK(!b.append_uint(12345678, 10));
        CHECK(!b.is_ok());

        UNSIGNED_LONGS_EQUAL(Size + 4, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // zero bytes left
        enum { Size = 9 };

        char dst[] = "xxxxxxxx";
        char res[] = "........";

        StringBuilder b(dst, Size);

        CHECK(b.append_str("........"));
        CHECK(b.is_ok());

        CHECK(!b.append_uint(1234, 10));
        CHECK(!b.is_ok());

        UNSIGNED_LONGS_EQUAL(Size + 4, b.needed_size());
        UNSIGNED_LONGS_EQUAL(Size, b.actual_size());

        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
}

TEST(string_builder, resizing) {
    { // assign
        StringBuffer buf(arena);
        StringBuilder b(buf);

        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(1, b.actual_size());

        UNSIGNED_LONGS_EQUAL(0, buf.len());
        STRCMP_EQUAL("", buf.c_str());

        CHECK(b.rewrite("1234"));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(5, b.needed_size());
        UNSIGNED_LONGS_EQUAL(5, b.actual_size());

        UNSIGNED_LONGS_EQUAL(4, buf.len());
        STRCMP_EQUAL("1234", buf.c_str());

        CHECK(b.rewrite("1234abcd"));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(9, b.needed_size());
        UNSIGNED_LONGS_EQUAL(9, b.actual_size());

        UNSIGNED_LONGS_EQUAL(8, buf.len());
        STRCMP_EQUAL("1234abcd", buf.c_str());
    }
    { // append
        StringBuffer buf(arena);
        StringBuilder b(buf);

        UNSIGNED_LONGS_EQUAL(1, b.needed_size());
        UNSIGNED_LONGS_EQUAL(1, b.actual_size());

        UNSIGNED_LONGS_EQUAL(0, buf.len());
        STRCMP_EQUAL("", buf.c_str());

        CHECK(b.append_str("1234"));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(5, b.needed_size());
        UNSIGNED_LONGS_EQUAL(5, b.actual_size());

        UNSIGNED_LONGS_EQUAL(4, buf.len());
        STRCMP_EQUAL("1234", buf.c_str());

        CHECK(b.append_str("abcd"));
        CHECK(b.is_ok());

        UNSIGNED_LONGS_EQUAL(9, b.needed_size());
        UNSIGNED_LONGS_EQUAL(9, b.actual_size());

        UNSIGNED_LONGS_EQUAL(8, buf.len());
        STRCMP_EQUAL("1234abcd", buf.c_str());
    }
}

} // namespace core
} // namespace roc
