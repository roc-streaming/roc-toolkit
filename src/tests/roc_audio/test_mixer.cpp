/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/mixer.h"

#include "test_stream_reader.h"
#include "test_helpers.h"

namespace roc {
namespace test {

using namespace audio;

namespace {

const size_t TEST_BUFSZ = 100;

const size_t TEST_MAX_SAMPLES = 1000;

} // namespace

TEST_GROUP(mixer) {
    TestStreamReader<TEST_MAX_SAMPLES> reader1;
    TestStreamReader<TEST_MAX_SAMPLES> reader2;

    Mixer mixer;

    void expect_output(size_t sz, int value) {
        read_buffers<TEST_MAX_SAMPLES>(mixer, 1, sz, value);
    }
};

TEST(mixer, no_readers) {
    expect_output(TEST_BUFSZ, 0);
}

TEST(mixer, one_reader) {
    mixer.add(reader1);

    reader1.add(TEST_BUFSZ, 111);

    expect_output(TEST_BUFSZ, 111);
}

TEST(mixer, two_readers) {
    mixer.add(reader1);
    mixer.add(reader2);

    reader1.add(TEST_BUFSZ, 111);
    reader2.add(TEST_BUFSZ, 222);

    expect_output(TEST_BUFSZ, 333);
}

TEST(mixer, remove_reader) {
    mixer.add(reader1);
    mixer.add(reader2);

    reader1.add(TEST_BUFSZ, 11);
    reader2.add(TEST_BUFSZ, 22);
    expect_output(TEST_BUFSZ, 33);

    mixer.remove(reader2);

    reader1.add(TEST_BUFSZ, 44);
    reader2.add(TEST_BUFSZ, 55);
    expect_output(TEST_BUFSZ, 44);

    mixer.remove(reader1);

    reader1.add(TEST_BUFSZ, 77);
    reader2.add(TEST_BUFSZ, 88);
    expect_output(TEST_BUFSZ, 0);
}

} // namespace test
} // namespace roc
