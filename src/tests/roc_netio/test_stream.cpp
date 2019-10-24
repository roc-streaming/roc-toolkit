/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_netio/stream.h"
#include "roc_netio/stream_buffer.h"

namespace roc {
namespace netio {

namespace {

StreamBufferPtr make_stream_buffer(core::IAllocator& allocator) {
    return new (allocator) StreamBuffer(allocator);
}

core::HeapAllocator allocator;

} // namespace

TEST_GROUP(stream) {};

TEST(stream, init) {
    Stream stream;
    CHECK(!stream.size());
}

TEST(stream, read) {
    { // read the whole stream
        StreamBufferPtr buf1 = make_stream_buffer(allocator);
        StreamBufferPtr buf2 = make_stream_buffer(allocator);

        CHECK(buf1->resize(strlen("foo")));
        CHECK(buf2->resize(strlen("bar")));

        Stream stream;

        stream.append(buf1);
        stream.append(buf2);

        memcpy(buf1->data(), "foo", strlen("foo"));
        memcpy(buf2->data(), "bar", strlen("bar"));

        const size_t size = strlen("foo") + strlen("bar");
        char buf[size];

        CHECK(stream.read(buf, sizeof(buf)) == sizeof(buf));
        CHECK(memcmp("foobar", buf, sizeof(buf)) == 0);
    }
    { // read buffer by buffer
        StreamBufferPtr buf1 = make_stream_buffer(allocator);
        StreamBufferPtr buf2 = make_stream_buffer(allocator);

        CHECK(buf1->resize(strlen("foo")));
        CHECK(buf2->resize(strlen("bar")));

        Stream stream;

        stream.append(buf1);
        stream.append(buf2);

        memcpy(buf1->data(), "foo", strlen("foo"));
        memcpy(buf2->data(), "bar", strlen("bar"));

        { // read first buffer
            const size_t size = strlen("foo");
            char buf[size];

            CHECK(stream.read(buf, sizeof(buf)) == sizeof(buf));
            CHECK(memcmp("foo", buf, sizeof(buf)) == 0);

            CHECK(strlen("bar") == stream.size());
        }
        { // read second buffer
            const size_t size = strlen("bar");
            char buf[size];

            CHECK(stream.read(buf, sizeof(buf)) == sizeof(buf));
            CHECK(memcmp("bar", buf, sizeof(buf)) == 0);

            CHECK(!stream.size());
        }
    }
    { // read partial of each buffer (fo + ob + ar)
        StreamBufferPtr buf1 = make_stream_buffer(allocator);
        StreamBufferPtr buf2 = make_stream_buffer(allocator);

        CHECK(buf1->resize(strlen("foo")));
        CHECK(buf2->resize(strlen("bar")));

        Stream stream;

        stream.append(buf1);
        stream.append(buf2);

        memcpy(buf1->data(), "foo", strlen("foo"));
        memcpy(buf2->data(), "bar", strlen("bar"));

        const size_t size = strlen("foo") + strlen("bar");

        { // read from first buffer
            char buf[2];

            CHECK(stream.read(buf, sizeof(buf)) == sizeof(buf));
            CHECK(memcmp("fo", buf, sizeof(buf)) == 0);

            CHECK(stream.size() == size - strlen("fo"));
        }
        { // read from first and second buffers
            char buf[2];

            CHECK(stream.read(buf, sizeof(buf)) == sizeof(buf));
            CHECK(memcmp("ob", buf, sizeof(buf)) == 0);

            CHECK(stream.size() == size - strlen("foob"));
        }
        { // read remaining part of second buffer
            char buf[2];

            CHECK(stream.read(buf, sizeof(buf)) == sizeof(buf));
            CHECK(memcmp("ar", buf, sizeof(buf)) == 0);

            CHECK(stream.size() == size - strlen("foobar"));
        }
    }
    { // read overflow
        StreamBufferPtr buf1 = make_stream_buffer(allocator);
        StreamBufferPtr buf2 = make_stream_buffer(allocator);

        CHECK(buf1->resize(strlen("foo")));
        CHECK(buf2->resize(strlen("bar")));

        Stream stream;

        stream.append(buf1);
        stream.append(buf2);

        memcpy(buf1->data(), "foo", strlen("foo"));
        memcpy(buf2->data(), "bar", strlen("bar"));

        char buf[strlen("foo") + strlen("bar")];

        CHECK(stream.read(buf, sizeof(buf) * 10) == sizeof(buf));
        CHECK(!stream.size());
    }
    { // read empty
        Stream stream;

        char buf[1];

        CHECK(stream.read(buf, 0) == -1);
        CHECK(!stream.size());
    }
}

} // namespace netio
} // namespace roc
