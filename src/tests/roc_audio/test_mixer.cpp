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
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/stddefs.h"

#include "test_mock_reader.h"

namespace roc {
namespace audio {

namespace {

enum { BufSz = 100, MaxSz = 1000 };

core::HeapAllocator allocator;
core::BufferPool<sample_t> buffer_pool(allocator, MaxSz, 1);

} // namespace

TEST_GROUP(mixer) {
    core::Slice<sample_t> new_buffer(size_t sz) {
        core::Slice<sample_t> buf = new (buffer_pool) core::Buffer<sample_t>(buffer_pool);
        buf.resize(sz);
        return buf;
    }

    void expect_output(Mixer& mixer, size_t sz, sample_t value) {
        Frame frame;
        frame.samples = new_buffer(sz);

        mixer.read(frame);

        UNSIGNED_LONGS_EQUAL(sz, frame.samples.size());

        for (size_t n = 0; n < sz; n++) {
            DOUBLES_EQUAL(value, frame.samples.data()[n], 0.0001);
        }
    }
};

TEST(mixer, no_readers) {
    Mixer mixer(buffer_pool);

    expect_output(mixer, BufSz, 0);
}

TEST(mixer, one_reader) {
    MockReader reader1;

    Mixer mixer(buffer_pool);

    mixer.add(reader1);

    reader1.add(BufSz, 0.11f);
    expect_output(mixer, BufSz, 0.11f);

    CHECK(reader1.num_unread() == 0);
}

TEST(mixer, two_readers) {
    MockReader reader1;
    MockReader reader2;

    Mixer mixer(buffer_pool);

    mixer.add(reader1);
    mixer.add(reader2);

    reader1.add(BufSz, 0.11f);
    reader2.add(BufSz, 0.22f);

    expect_output(mixer, BufSz, 0.33f);

    CHECK(reader1.num_unread() == 0);
    CHECK(reader2.num_unread() == 0);
}

TEST(mixer, remove_reader) {
    MockReader reader1;
    MockReader reader2;

    Mixer mixer(buffer_pool);

    mixer.add(reader1);
    mixer.add(reader2);

    reader1.add(BufSz, 0.11f);
    reader2.add(BufSz, 0.22f);
    expect_output(mixer, BufSz, 0.33f);

    mixer.remove(reader2);

    reader1.add(BufSz, 0.44f);
    reader2.add(BufSz, 0.55f);
    expect_output(mixer, BufSz, 0.44f);

    mixer.remove(reader1);

    reader1.add(BufSz, 0.77f);
    reader2.add(BufSz, 0.88f);
    expect_output(mixer, BufSz, 0.0f);

    CHECK(reader1.num_unread() == BufSz);
    CHECK(reader2.num_unread() == BufSz * 2);
}

TEST(mixer, clamp) {
    MockReader reader1;
    MockReader reader2;

    Mixer mixer(buffer_pool);

    mixer.add(reader1);
    mixer.add(reader2);

    reader1.add(BufSz, 0.900f);
    reader2.add(BufSz, 0.101f);

    expect_output(mixer, BufSz, 1.0f);

    reader1.add(BufSz, 0.2f);
    reader2.add(BufSz, 1.1f);

    expect_output(mixer, BufSz, 1.0f);

    reader1.add(BufSz, -0.2f);
    reader2.add(BufSz, -0.81f);

    expect_output(mixer, BufSz, -1.0f);

    CHECK(reader1.num_unread() == 0);
    CHECK(reader2.num_unread() == 0);
}

} // namespace audio
} // namespace roc
