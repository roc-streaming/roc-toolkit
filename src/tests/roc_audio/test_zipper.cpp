/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/zipper.h"

#include "test_stream_reader.h"

namespace roc {
namespace test {

using namespace audio;

namespace {

enum { BufSz = 100, MaxSamples = 1000 };

typedef TestStreamReader<MaxSamples> TestReader;

} // namespace

TEST_GROUP(zipper) {
    TestReader reader1;
    TestReader reader2;

    Zipper zipper;

    void add_input(TestReader & reader, size_t number) {
        for (size_t n = 0; n < BufSz; n++) {
            reader.add(1, int(n * number));
        }
    }

    void expect_output(ISampleBuffer & buf, size_t total, size_t number) {
        for (size_t n = 0; n < BufSz; n++) {
            long actual = (long)buf.data()[n * total + number - 1];
            long expected = (long)(n * number);

            LONGS_EQUAL(expected, actual);
        }
    }

    ISampleBufferPtr read_buffer(size_t bufsz) {
        ISampleBufferPtr buf = new_buffer<MaxSamples>(bufsz);

        zipper.read(*buf);
        return buf;
    }
};

TEST(zipper, no_readers) {
    ISampleBufferPtr buf = read_buffer(BufSz);

    expect_data(buf->data(), BufSz, 0);
}

TEST(zipper, one_reader) {
    zipper.add(reader1);

    add_input(reader1, 1);

    ISampleBufferPtr buf = read_buffer(BufSz);

    expect_output(*buf, 1, 1);
}

TEST(zipper, two_readers) {
    zipper.add(reader1);
    zipper.add(reader2);

    add_input(reader1, 1);
    add_input(reader2, 2);

    ISampleBufferPtr buf = read_buffer(BufSz * 2);

    expect_output(*buf, 2, 1);
    expect_output(*buf, 2, 2);
}

TEST(zipper, remove_reader) {
    zipper.add(reader1);
    zipper.add(reader2);

    add_input(reader1, 1);
    add_input(reader2, 2);

    read_buffer(BufSz * 2);

    zipper.remove(reader2);

    add_input(reader1, 1);
    add_input(reader2, 2);

    ISampleBufferPtr buf = read_buffer(BufSz);

    expect_output(*buf, 1, 1);
}

} // namespace test
} // namespace roc
