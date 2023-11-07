/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/printer.h"

namespace roc {
namespace core {

namespace {

char buffer[Printer::BufferSize * 10];

void buffer_println(const char* buf, size_t bufsz) {
    CHECK(strlen(buffer) + bufsz < sizeof(buffer));
    size_t pos = strlen(buffer);
    memcpy(buffer + pos, buf, bufsz);
    pos += bufsz;
    buffer[pos++] = '\n';
    buffer[pos++] = '\0';
}

} // namespace

// clang-format off
TEST_GROUP(printer) {
    void setup() {
        memset(buffer, 0, sizeof(buffer));
    }
};
// clang-format on

TEST(printer, one_line) {
    Printer p(&buffer_println);

    UNSIGNED_LONGS_EQUAL(6, p.writef("%s", "hello\n"));
    STRCMP_EQUAL("hello\n", buffer);
}

TEST(printer, two_lines_in_one_write) {
    Printer p(&buffer_println);

    UNSIGNED_LONGS_EQUAL(12, p.writef("%s", "hello\nworld\n"));
    STRCMP_EQUAL("hello\nworld\n", buffer);
}

TEST(printer, many_lines) {
    Printer p(&buffer_println);

    char text[sizeof(buffer)] = {};

    for (int i = 0; i < 500; i++) {
        strcat(text, "123456789\n");
        UNSIGNED_LONGS_EQUAL(10, p.writef("%s\n", "123456789"));
    }

    STRCMP_EQUAL(text, buffer);
}

TEST(printer, many_lines_varying_size) {
    Printer p(&buffer_println);

    char text[sizeof(buffer)] = {};

    for (int i = 0; i < 100; i++) {
        char t[101] = {};
        for (int j = 0; j < i; j++) {
            strcat(t, "x");
        }

        strcat(text, t);
        strcat(text, "\n");

        UNSIGNED_LONGS_EQUAL(strlen(t) + 1, p.writef("%s\n", t));
    }

    CHECK(strlen(text) > Printer::BufferSize * 3);
    CHECK(strlen(text) < sizeof(buffer));

    STRCMP_EQUAL(text, buffer);
}

TEST(printer, whole_buffer_many_writes) {
    Printer p(&buffer_println);

    for (int i = 0; i < Printer::BufferSize - 1; i++) {
        UNSIGNED_LONGS_EQUAL(1, p.writef("x"));
    }

    STRCMP_EQUAL("", buffer);

    UNSIGNED_LONGS_EQUAL(1, p.writef("\n"));

    char text[sizeof(buffer)] = {};
    for (int i = 0; i < Printer::BufferSize - 1; i++) {
        strcat(text, "x");
    }
    strcat(text, "\n");

    STRCMP_EQUAL(text, buffer);
}

TEST(printer, whole_buffer_one_write) {
    Printer p(&buffer_println);

    char text[sizeof(buffer)] = {};
    for (int i = 0; i < Printer::BufferSize - 1; i++) {
        strcat(text, "x");
    }
    strcat(text, "\n");

    UNSIGNED_LONGS_EQUAL(strlen(text), p.writef("%s", text));

    STRCMP_EQUAL(text, buffer);
}

TEST(printer, flush_on_destroy) {
    {
        Printer p(&buffer_println);

        UNSIGNED_LONGS_EQUAL(5, p.writef("%s", "hello"));
        STRCMP_EQUAL("", buffer);
    }

    STRCMP_EQUAL("hello\n", buffer);
}

} // namespace core
} // namespace roc
