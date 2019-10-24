/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/string_utils.h"

namespace roc {
namespace core {

TEST_GROUP(string_utils) {};

TEST(string_utils, copy_str) {
    { // copy exact size
        enum { Len = 8 };

        char dst[] = "xxxxxxxx";
        char src[] = "12345678";
        char res[] = "12345678";

        CHECK(copy_str(dst, Len + 1, src, src + strlen(src)));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // copy smaller size
        enum { Len = 8 };

        char dst[] = "xxxxxxxx";
        char src[] = "123456789abcd";
        char res[] = "1234\0xxx";

        CHECK(copy_str(dst, Len + 1, src, src + 4));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // copy larger size (truncation)
        enum { Len = 8 };

        char dst[] = "xxxxxxxx";
        char src[] = "123456789abcd";
        char res[] = "12345678";

        CHECK(!copy_str(dst, Len + 1, src, src + strlen(src)));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // dst is one byte
        enum { Len = 0 };

        char dst[] = "xx";
        char src[] = "12345678";
        char res[] = "\0x";

        CHECK(!copy_str(dst, Len + 1, src, src + strlen(src)));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // dst is zero bytes
        char dst[] = "xx";
        char src[] = "12345678";

        CHECK(!copy_str(dst, 0, src, src + strlen(src)));
        STRCMP_EQUAL("xx", dst);
    }
}

TEST(string_utils, append_str) {
    { // append exact size
        enum { Len = 8 };

        char dst[] = "xxxx\0xxx";
        char src[] = "1234";
        char res[] = "xxxx1234";

        CHECK(append_str(dst, Len + 1, src, src + strlen(src)));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append smaller size
        enum { Len = 8 };

        char dst[] = "xx\0xxxxx";
        char src[] = "12345678";
        char res[] = "xx123\0xx";

        CHECK(append_str(dst, Len + 1, src, src + 3));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append larger size (truncation)
        enum { Len = 8 };

        char dst[] = "xx\0xxxxx";
        char src[] = "12345678";
        char res[] = "xx123456";

        CHECK(!append_str(dst, Len + 1, src, src + strlen(src)));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // dst is one byte
        enum { Len = 0 };

        char dst[] = "";
        char src[] = "12345678";

        CHECK(!append_str(dst, Len + 1, src, src + strlen(src)));
        STRCMP_EQUAL("", dst);
    }
    { // dst is zero bytes
        char dst[] = "xx";
        char src[] = "12345678";

        CHECK(!append_str(dst, 0, src, src + strlen(src)));
        STRCMP_EQUAL("xx", dst);
    }
}

TEST(string_utils, append_uint) {
    { // append exact size (10 base)
        enum { Len = 8 };

        char dst[] = "xxxx\0xxx";
        char res[] = "xxxx1234";

        CHECK(append_uint(dst, Len + 1, 1234, 10));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append exact size (16 base)
        enum { Len = 8 };

        char dst[] = "xxxx\0dead";
        char res[] = "xxxxdead";

        CHECK(append_uint(dst, Len + 1, 0xdead, 16));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append smaller size
        enum { Len = 8 };

        char dst[] = "xx\0xxxxx";
        char res[] = "xx123\0xx";

        CHECK(append_uint(dst, Len + 1, 123, 10));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append smaller size (zero)
        enum { Len = 8 };

        char dst[] = "xx\0xxxxx";
        char res[] = "xx0\0xxxx";

        CHECK(append_uint(dst, Len + 1, 0, 10));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // append larger size (truncation)
        enum { Len = 8 };

        char dst[] = "xx\0xxxxx";
        char res[] = "xx123456";

        CHECK(!append_uint(dst, Len + 1, 12345678, 10));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // dst is one byte
        enum { Len = 0 };

        char dst[] = "\0x";
        char res[] = "\0x";

        CHECK(!append_uint(dst, Len + 1, 12345678, 10));
        CHECK(memcmp(dst, res, sizeof(res)) == 0);
    }
    { // dst is zero bytes
        char dst[] = "xx";

        CHECK(!append_uint(dst, 0, 12345678, 10));
        STRCMP_EQUAL("xx", dst);
    }
}

} // namespace core
} // namespace roc
