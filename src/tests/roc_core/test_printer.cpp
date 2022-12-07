/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>
#include <CppUTest/UtestMacros.h>

#include "roc_core/printer.h"

namespace roc {
namespace core {

namespace {

char buffer[Printer::BufferSize * 10];

void buffer_print(const char* buf, size_t bufsz) {
    CHECK(strlen(buffer) + bufsz < sizeof(buffer));
    memcpy(buffer + strlen(buffer), buf, bufsz);
}

} // namespace

// clang-format off
TEST_GROUP(printer) {
    void setup() {
        memset(buffer, 0, sizeof(buffer));
    }
};
// clang-format on

TEST(printer, write_flush) {
    Printer p(&buffer_print);

    UNSIGNED_LONGS_EQUAL(5, p.writef("%s", "hello"));
    STRCMP_EQUAL("", buffer);

    p.flush();
    STRCMP_EQUAL("hello", buffer);
}

TEST(printer, write_destroy) {
    {
        Printer p(&buffer_print);

        UNSIGNED_LONGS_EQUAL(5, p.writef("%s", "hello"));
        STRCMP_EQUAL("", buffer);
    }

    STRCMP_EQUAL("hello", buffer);
}

TEST(printer, write_many) {
    Printer p(&buffer_print);

    for (int i = 0; i < Printer::FlushThreshold - 1; i++) {
        UNSIGNED_LONGS_EQUAL(1, p.writef("x"));
    }

    STRCMP_EQUAL("", buffer);

    UNSIGNED_LONGS_EQUAL(1, p.writef("x"));

    char text[sizeof(buffer)] = {};
    for (int i = 0; i < Printer::FlushThreshold; i++) {
        strcat(text, "x");
    }

    STRCMP_EQUAL(text, buffer);
}

TEST(printer, write_big) {
    Printer p(&buffer_print);

    char text[sizeof(buffer)] = {};
    for (int i = 0; i < Printer::FlushThreshold; i++) {
        strcat(text, "x");
    }

    UNSIGNED_LONGS_EQUAL(strlen(text), p.writef("%s", text));

    STRCMP_EQUAL(text, buffer);
}

TEST(printer, overflow) {
    Printer p(&buffer_print);

    char text_concat[sizeof(buffer)] = {};

    char text1[sizeof(buffer)] = {};
    for (int i = 0; i < Printer::FlushThreshold - 10; i++) {
        strcat(text1, "x");
        strcat(text_concat, "x");
    }

    UNSIGNED_LONGS_EQUAL(strlen(text1), p.writef("%s", text1));

    STRCMP_EQUAL("", buffer);

    char text2[sizeof(buffer)] = {};
    for (int i = 0; i < Printer::FlushThreshold - 10; i++) {
        strcat(text2, "y");
        strcat(text_concat, "y");
    }

    UNSIGNED_LONGS_EQUAL(strlen(text2), p.writef("%s", text2));

    STRCMP_EQUAL(text1, buffer);

    p.flush();

    STRCMP_EQUAL(text_concat, buffer);
}

TEST(printer, truncation) {
    Printer p(&buffer_print);

    char text_truncated[sizeof(buffer)] = {};

    char text[sizeof(buffer)] = {};

    for (int i = 0; i < Printer::BufferSize * 2; i++) {
        strcat(text, "x");
        if (i < Printer::BufferSize) {
            strcat(text_truncated, "x");
        }
    }

    UNSIGNED_LONGS_EQUAL(strlen(text_truncated), p.writef("%s", text));

    STRCMP_EQUAL(text_truncated, buffer);

    p.flush();

    STRCMP_EQUAL(text_truncated, buffer);
}

TEST(printer, newline) {
    Printer p(&buffer_print);

    char text[sizeof(buffer)] = {};
    for (int i = 0; i < Printer::FlushThreshold - 10; i++) {
        strcat(text, "x");
    }
    strcat(text, "\n");

    UNSIGNED_LONGS_EQUAL(strlen(text), p.writef("%s", text));

    STRCMP_EQUAL("", buffer);

    UNSIGNED_LONGS_EQUAL(9, p.writef("123456789"));

    STRCMP_EQUAL(text, buffer);

    p.flush();

    strcat(text, "123456789");

    STRCMP_EQUAL(text, buffer);
}

TEST(printer, newline_end) {
    Printer p(&buffer_print);

    char text[sizeof(buffer)] = {};
    for (int i = 0; i < Printer::FlushThreshold - 1; i++) {
        strcat(text, "x");
    }

    UNSIGNED_LONGS_EQUAL(strlen(text), p.writef("%s", text));

    STRCMP_EQUAL("", buffer);

    UNSIGNED_LONGS_EQUAL(1, p.writef("\n"));

    strcat(text, "\n");

    STRCMP_EQUAL(text, buffer);
}

TEST(printer, many_lines) {
    Printer p(&buffer_print);

    char text[sizeof(buffer)] = {};

    for (int i = 0; i < 500; i++) {
        strcat(text, "123456789\n");
        UNSIGNED_LONGS_EQUAL(10, p.writef("%s\n", "123456789"));
    }

    p.flush();

    STRCMP_EQUAL(text, buffer);
}

TEST(printer, varying_size) {
    Printer p(&buffer_print);

    char text[sizeof(buffer)] = {};

    for (int i = 0; i < 200; i++) {
        char t[100] = {};
        for (int i = 0; i < 40; i++) {
            strcat(t, "x");
        }

        strcat(text, t);
        strcat(text, "\n");

        UNSIGNED_LONGS_EQUAL(strlen(t) + 1, p.writef("%s\n", t));
    }

    p.flush();

    CHECK(strlen(text) > Printer::BufferSize * 3);
    CHECK(strlen(text) < sizeof(buffer));

    STRCMP_EQUAL(text, buffer);
}

} // namespace core
} // namespace roc
