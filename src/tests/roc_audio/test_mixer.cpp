/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_reader.h"

#include "roc_audio/mixer.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

enum { BufSz = 100, SampleRate = 44100,
       ChannelMask = 0x1, MaxBufSz = 500 };

const core::nanoseconds_t MaxBufDuration =
    MaxBufSz * core::Second / (SampleRate * packet::num_channels(ChannelMask));

core::HeapAllocator allocator;
core::BufferPool<sample_t> buffer_pool(allocator, MaxBufSz, true);
core::BufferPool<sample_t> large_buffer_pool(allocator, MaxBufSz * 10, true);

} // namespace

TEST_GROUP(mixer) {
    core::Slice<sample_t> new_buffer(size_t sz) {
        core::Slice<sample_t> buf =
            new (large_buffer_pool) core::Buffer<sample_t>(large_buffer_pool);
        buf.resize(sz);
        return buf;
    }

    void expect_output(Mixer& mixer, size_t sz, sample_t value) {
        core::Slice<sample_t> buf = new_buffer(sz);

        Frame frame(buf.data(), buf.size());
        CHECK(mixer.read(frame));

        for (size_t n = 0; n < sz; n++) {
            DOUBLES_EQUAL((double)value, (double)frame.data()[n], 0.0001);
        }
    }
};

TEST(mixer, no_readers) {
    Mixer mixer(buffer_pool, MaxBufDuration, SampleRate, ChannelMask);
    CHECK(mixer.valid());

    expect_output(mixer, BufSz, 0);
}

TEST(mixer, one_reader) {
    test::MockReader reader;

    Mixer mixer(buffer_pool, MaxBufDuration, SampleRate, ChannelMask);
    CHECK(mixer.valid());

    mixer.add_input(reader);

    reader.add(BufSz, 0.11f);
    expect_output(mixer, BufSz, 0.11f);

    CHECK(reader.num_unread() == 0);
}

TEST(mixer, one_reader_large) {
    test::MockReader reader;

    Mixer mixer(buffer_pool, MaxBufDuration, SampleRate, ChannelMask);
    CHECK(mixer.valid());

    mixer.add_input(reader);

    reader.add(MaxBufSz * 2, 0.11f);
    expect_output(mixer, MaxBufSz * 2, 0.11f);

    CHECK(reader.num_unread() == 0);
}

TEST(mixer, two_readers) {
    test::MockReader reader1;
    test::MockReader reader2;

    Mixer mixer(buffer_pool, MaxBufDuration, SampleRate, ChannelMask);
    CHECK(mixer.valid());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.add(BufSz, 0.11f);
    reader2.add(BufSz, 0.22f);

    expect_output(mixer, BufSz, 0.33f);

    CHECK(reader1.num_unread() == 0);
    CHECK(reader2.num_unread() == 0);
}

TEST(mixer, remove_reader) {
    test::MockReader reader1;
    test::MockReader reader2;

    Mixer mixer(buffer_pool, MaxBufDuration, SampleRate, ChannelMask);
    CHECK(mixer.valid());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.add(BufSz, 0.11f);
    reader2.add(BufSz, 0.22f);
    expect_output(mixer, BufSz, 0.33f);

    mixer.remove_input(reader2);

    reader1.add(BufSz, 0.44f);
    reader2.add(BufSz, 0.55f);
    expect_output(mixer, BufSz, 0.44f);

    mixer.remove_input(reader1);

    reader1.add(BufSz, 0.77f);
    reader2.add(BufSz, 0.88f);
    expect_output(mixer, BufSz, 0.0f);

    CHECK(reader1.num_unread() == BufSz);
    CHECK(reader2.num_unread() == BufSz * 2);
}

TEST(mixer, clamp) {
    test::MockReader reader1;
    test::MockReader reader2;

    Mixer mixer(buffer_pool, MaxBufDuration, SampleRate, ChannelMask);
    CHECK(mixer.valid());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

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
