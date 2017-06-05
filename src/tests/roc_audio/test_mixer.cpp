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

#include "test_helpers.h"
#include "test_stream_reader.h"

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

    void expect_output(size_t sz, packet::sample_t value) {
        read_buffers<MaxSamples>(mixer, 1, sz, value);
    }
};

TEST(mixer, no_readers) {
    expect_output(BufSz, 0);
}

TEST(mixer, one_reader) {
    mixer.add(reader1);

    reader1.add(BufSz, 0.111f);

    expect_output(BufSz, 0.111f);
}

TEST(mixer, two_readers) {
    mixer.add(reader1);
    mixer.add(reader2);

    reader1.add(BufSz, 0.111f);
    reader2.add(BufSz, 0.222f);

    expect_output(BufSz, 0.333f);
}

TEST(mixer, remove_reader) {
    mixer.add(reader1);
    mixer.add(reader2);

    reader1.add(BufSz, 0.11f);
    reader2.add(BufSz, 0.22f);
    expect_output(BufSz, 0.33f);

    mixer.remove(reader2);

    reader1.add(BufSz, 0.44f);
    reader2.add(BufSz, 0.55f);
    expect_output(BufSz, 0.44f);

    mixer.remove(reader1);

    reader1.add(BufSz, 0.77f);
    reader2.add(BufSz, 0.88f);
    expect_output(BufSz, 0.0f);
}

TEST(mixer, clamp) {
    mixer.add(reader1);
    mixer.add(reader2);

    reader1.add(BufSz, 0.900f);
    reader2.add(BufSz, 0.101f);

    expect_output(BufSz, 1.0f);

    reader1.add(BufSz, 0.2f);
    reader2.add(BufSz, 1.1f);

    expect_output(BufSz, 1.0f);

    reader1.add(BufSz, -0.2f);
    reader2.add(BufSz, -0.81f);

    expect_output(BufSz, -1.0f);
}

} // namespace test
} // namespace roc
