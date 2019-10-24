/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_netio/stream_buffer.h"

namespace roc {
namespace netio {

namespace {

core::HeapAllocator allocator;

} // namespace

TEST_GROUP(stream_buffer) {};

TEST(stream_buffer, init) {
    StreamBuffer buffer(allocator);

    CHECK(buffer.size() == 0);
}

TEST(stream_buffer, write_bytes) {
    StreamBuffer buffer(allocator);

    CHECK(buffer.resize(strlen("foo")));
    CHECK(buffer.size() == strlen("foo"));

    memcpy(buffer.data(), "foo", strlen("foo"));
}

TEST(stream_buffer, read_bytes) {
    StreamBuffer buffer(allocator);
    CHECK(buffer.resize(strlen("foobar")));

    memcpy(buffer.data(), "foobar", strlen("foobar"));

    { // read zero bytes
        char buf[strlen("foo")];

        CHECK(buffer.read(buf, 0) == -1);
        CHECK(buffer.size() == strlen("foobar"));
    }
    { // read first part
        char buf[strlen("foo")];

        CHECK(buffer.read(buf, sizeof(buf)) == strlen("foo"));
        CHECK(memcmp("foo", buf, sizeof(buf)) == 0);

        CHECK(buffer.size() == strlen("bar"));
    }
    { // read second part
        char buf[strlen("bar")];

        CHECK(buffer.read(buf, sizeof(buf)) == strlen("bar"));
        CHECK(memcmp("bar", buf, sizeof(buf)) == 0);

        CHECK(buffer.size() == 0);
    }
    { // try to read first part again
        char buf[strlen("foo")];

        CHECK(buffer.read(buf, strlen("foo")) == -1);
        CHECK(buffer.size() == 0);
    }
}

TEST(stream_buffer, read_bytes_overflow) {
    { // read whole data stream
        StreamBuffer buffer(allocator);

        CHECK(buffer.resize(strlen("foobar")));
        memcpy(buffer.data(), "foobar", strlen("foobar"));

        const size_t len = strlen("foobar");
        char buf[len];

        CHECK(buffer.read(buf, len * 2));
        CHECK(memcmp("foobar", buf, sizeof(buf)) == 0);

        CHECK(buffer.size() == 0);
    }
    { // read some data first to have an offset and try to overflow next
        StreamBuffer buffer(allocator);

        CHECK(buffer.resize(strlen("foobar")));
        memcpy(buffer.data(), "foobar", strlen("foobar"));

        const size_t size = strlen("foobar");

        {
            char buf[1];

            CHECK(buffer.read(buf, sizeof(buf)));
            CHECK(memcmp("f", buf, sizeof(buf)) == 0);

            CHECK(buffer.size() == size - 1);
        }
        {
            char buf[size - 1];

            CHECK(buffer.read(buf, buffer.size() * 10));
            CHECK(memcmp("oobar", buf, sizeof(buf)) == 0);

            CHECK(buffer.size() == 0);
        }
    }
}

} // namespace netio
} // namespace roc
