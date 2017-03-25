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

enum { BufSz = 100, MaxSamples = 1000 };

} // namespace

TEST_GROUP(mixer) {
    TestStreamReader<MaxSamples> reader1;
    TestStreamReader<MaxSamples> reader2;

    Mixer mixer;

    void expect_output(size_t sz, int value) {
        read_buffers<MaxSamples>(mixer, 1, sz, value);
    }
};

TEST(mixer, no_readers) {
    expect_output(BufSz, 0);
}

TEST(mixer, one_reader) {
    mixer.add(reader1);

    reader1.add(BufSz, 0.111);

    expect_output(BufSz, 0.111);
}

TEST(mixer, two_readers) {
    mixer.add(reader1);
    mixer.add(reader2);

    reader1.add(BufSz, 0.111);
    reader2.add(BufSz, 0.222);

    expect_output(BufSz, 0.333);
}

TEST(mixer, remove_reader) {
    mixer.add(reader1);
    mixer.add(reader2);

    reader1.add(BufSz, 0.11);
    reader2.add(BufSz, 0.22);
    expect_output(BufSz, 0.33);

    mixer.remove(reader2);

    reader1.add(BufSz, 0.44);
    reader2.add(BufSz, 0.55);
    expect_output(BufSz, 0.44);

    mixer.remove(reader1);

    reader1.add(BufSz, 0.77);
    reader2.add(BufSz, 0.88);
    expect_output(BufSz, 0.0);
}

} // namespace test
} // namespace roc
