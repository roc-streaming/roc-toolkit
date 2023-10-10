/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/time.h"
#include "test_helpers/mock_reader.h"

#include "roc_audio/mixer.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

enum { BufSz = 100, SampleRate = 44100, ChannelMask = 0x1, MaxBufSz = 500 };

core::HeapArena arena;
core::BufferFactory<sample_t> buffer_factory(arena, MaxBufSz);
core::BufferFactory<sample_t> large_buffer_factory(arena, MaxBufSz * 10);

core::Slice<sample_t> new_buffer(size_t sz) {
    core::Slice<sample_t> buf = large_buffer_factory.new_buffer();
    buf.reslice(0, sz);
    return buf;
}

void expect_output(Mixer& mixer,
                   size_t sz,
                   sample_t value,
                   unsigned flags = 0,
                   core::nanoseconds_t capture_ts = -1) {
    core::Slice<sample_t> buf = new_buffer(sz);

    Frame frame(buf.data(), buf.size());
    CHECK(mixer.read(frame));

    for (size_t n = 0; n < sz; n++) {
        DOUBLES_EQUAL((double)value, (double)frame.samples()[n], 0.0001);
    }

    UNSIGNED_LONGS_EQUAL(flags, frame.flags());

    if (capture_ts < 0) {
        LONGS_EQUAL(0, frame.capture_timestamp());
    } else {
        CHECK(core::ns_equal_delta(frame.capture_timestamp(), capture_ts,
                                   core::Microsecond));
    }
}

} // namespace

TEST_GROUP(mixer) {};

TEST(mixer, no_readers) {
    Mixer mixer(buffer_factory, true);
    CHECK(mixer.is_valid());

    expect_output(mixer, BufSz, 0);
}

TEST(mixer, one_reader) {
    test::MockReader reader;

    Mixer mixer(buffer_factory, true);
    CHECK(mixer.is_valid());

    mixer.add_input(reader);

    reader.add_samples(BufSz, 0.11f);
    expect_output(mixer, BufSz, 0.11f);

    CHECK(reader.num_unread() == 0);
}

TEST(mixer, one_reader_large) {
    test::MockReader reader;

    Mixer mixer(buffer_factory, true);
    CHECK(mixer.is_valid());

    mixer.add_input(reader);

    reader.add_samples(MaxBufSz * 2, 0.11f);
    expect_output(mixer, MaxBufSz * 2, 0.11f);

    CHECK(reader.num_unread() == 0);
}

TEST(mixer, two_readers) {
    test::MockReader reader1;
    test::MockReader reader2;

    Mixer mixer(buffer_factory, true);
    CHECK(mixer.is_valid());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.22f);

    expect_output(mixer, BufSz, 0.33f);

    CHECK(reader1.num_unread() == 0);
    CHECK(reader2.num_unread() == 0);
}

TEST(mixer, remove_reader) {
    test::MockReader reader1;
    test::MockReader reader2;

    Mixer mixer(buffer_factory, true);
    CHECK(mixer.is_valid());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.22f);
    expect_output(mixer, BufSz, 0.33f);

    mixer.remove_input(reader2);

    reader1.add_samples(BufSz, 0.44f);
    reader2.add_samples(BufSz, 0.55f);
    expect_output(mixer, BufSz, 0.44f);

    mixer.remove_input(reader1);

    reader1.add_samples(BufSz, 0.77f);
    reader2.add_samples(BufSz, 0.88f);
    expect_output(mixer, BufSz, 0.0f);

    CHECK(reader1.num_unread() == BufSz);
    CHECK(reader2.num_unread() == BufSz * 2);
}

TEST(mixer, clamp) {
    test::MockReader reader1;
    test::MockReader reader2;

    Mixer mixer(buffer_factory, true);
    CHECK(mixer.is_valid());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.add_samples(BufSz, 0.900f);
    reader2.add_samples(BufSz, 0.101f);

    expect_output(mixer, BufSz, 1.0f);

    reader1.add_samples(BufSz, 0.2f);
    reader2.add_samples(BufSz, 1.1f);

    expect_output(mixer, BufSz, 1.0f);

    reader1.add_samples(BufSz, -0.2f);
    reader2.add_samples(BufSz, -0.81f);

    expect_output(mixer, BufSz, -1.0f);

    CHECK(reader1.num_unread() == 0);
    CHECK(reader2.num_unread() == 0);
}

TEST(mixer, flags) {
    enum { BigBatch = MaxBufSz * 2 };

    test::MockReader reader1;
    test::MockReader reader2;

    Mixer mixer(buffer_factory, true);
    CHECK(mixer.is_valid());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.add_samples(BigBatch, 0.1f, 0);
    reader1.add_samples(BigBatch, 0.1f, Frame::FlagNonblank);
    reader1.add_samples(BigBatch, 0.1f, 0);

    reader2.add_samples(BigBatch, 0.1f, Frame::FlagIncomplete);
    reader2.add_samples(BigBatch / 2, 0.1f, 0);
    reader2.add_samples(BigBatch / 2, 0.1f, Frame::FlagDrops);
    reader2.add_samples(BigBatch, 0.1f, 0);

    expect_output(mixer, BigBatch, 0.2f, Frame::FlagIncomplete);
    expect_output(mixer, BigBatch, 0.2f, Frame::FlagNonblank | Frame::FlagDrops);
    expect_output(mixer, BigBatch, 0.2f, 0);

    CHECK(reader1.num_unread() == 0);
    CHECK(reader2.num_unread() == 0);
}

TEST(mixer, timestamps_one_reader) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, ChanLayout_Surround, ChanOrder_Smpte,
                                 ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts = 1000000000000;

    test::MockReader reader;

    Mixer mixer(buffer_factory, true);
    CHECK(mixer.is_valid());

    mixer.add_input(reader);

    reader.enable_timestamps(start_ts, sample_spec);

    reader.add_samples(BufSz, 0.11f);
    expect_output(mixer, BufSz, 0.11f, 0, start_ts);

    reader.add_samples(BufSz, 0.22f);
    expect_output(mixer, BufSz, 0.22f, 0, start_ts + core::Second);

    reader.add_samples(BufSz, 0.33f);
    expect_output(mixer, BufSz, 0.33f, 0, start_ts + core::Second * 2);

    CHECK(reader.num_unread() == 0);
}

TEST(mixer, timestamps_two_readers) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, ChanLayout_Surround, ChanOrder_Smpte,
                                 ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts1 = 2000000000000;
    const core::nanoseconds_t start_ts2 = 1000000000000;

    test::MockReader reader1;
    test::MockReader reader2;

    Mixer mixer(buffer_factory, true);
    CHECK(mixer.is_valid());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.enable_timestamps(start_ts1, sample_spec);
    reader2.enable_timestamps(start_ts2, sample_spec);

    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.11f);
    expect_output(mixer, BufSz, 0.11f * 2, 0, (start_ts1 + start_ts2) / 2);

    reader1.add_samples(BufSz, 0.22f);
    reader2.add_samples(BufSz, 0.22f);
    expect_output(mixer, BufSz, 0.22f * 2, 0,
                  ((start_ts1 + core::Second) + (start_ts2 + core::Second)) / 2);

    reader1.add_samples(BufSz, 0.33f);
    reader2.add_samples(BufSz, 0.33f);
    expect_output(mixer, BufSz, 0.33f * 2, 0,
                  ((start_ts1 + core::Second * 2) + (start_ts2 + core::Second * 2)) / 2);

    CHECK(reader1.num_unread() == 0);
    CHECK(reader2.num_unread() == 0);
}

TEST(mixer, timestamps_partial) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, ChanLayout_Surround, ChanOrder_Smpte,
                                 ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts1 = 2000000000000;
    const core::nanoseconds_t start_ts2 = 1000000000000;

    test::MockReader reader1;
    test::MockReader reader2;
    test::MockReader reader3;

    Mixer mixer(buffer_factory, true);
    CHECK(mixer.is_valid());

    mixer.add_input(reader1);
    mixer.add_input(reader2);
    mixer.add_input(reader3);

    reader1.enable_timestamps(start_ts1, sample_spec);
    reader2.enable_timestamps(start_ts2, sample_spec);
    // reader3 does not have timestamps

    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.11f);
    reader3.add_samples(BufSz, 0.11f);
    expect_output(mixer, BufSz, 0.11f * 3, 0, (start_ts1 + start_ts2) / 3);

    reader1.add_samples(BufSz, 0.22f);
    reader2.add_samples(BufSz, 0.22f);
    reader3.add_samples(BufSz, 0.22f);
    expect_output(mixer, BufSz, 0.22f * 3, 0,
                  ((start_ts1 + core::Second) + (start_ts2 + core::Second)) / 3);

    reader1.add_samples(BufSz, 0.33f);
    reader2.add_samples(BufSz, 0.33f);
    reader3.add_samples(BufSz, 0.33f);
    expect_output(mixer, BufSz, 0.33f * 3, 0,
                  ((start_ts1 + core::Second * 2) + (start_ts2 + core::Second * 2)) / 3);

    CHECK(reader1.num_unread() == 0);
    CHECK(reader2.num_unread() == 0);
    CHECK(reader3.num_unread() == 0);
}

TEST(mixer, timestamps_no_overflow) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, ChanLayout_Surround, ChanOrder_Smpte,
                                 ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts1 = 9000000000000000000ll;
    const core::nanoseconds_t start_ts2 = 9100000000000000000ll;

    // ensure there would be an overflow if we directly sum timestamps
    // mixer should produce correct results despite of that
    CHECK(int64_t(uint64_t(start_ts1) + uint64_t(start_ts2)) < 0);

    test::MockReader reader1;
    test::MockReader reader2;

    Mixer mixer(buffer_factory, true);
    CHECK(mixer.is_valid());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.enable_timestamps(start_ts1, sample_spec);
    reader2.enable_timestamps(start_ts2, sample_spec);

    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.11f);
    expect_output(mixer, BufSz, 0.11f * 2, 0, start_ts1 / 2 + start_ts2 / 2);

    reader1.add_samples(BufSz, 0.22f);
    reader2.add_samples(BufSz, 0.22f);
    expect_output(mixer, BufSz, 0.22f * 2, 0,
                  (start_ts1 + core::Second) / 2 + (start_ts2 + core::Second) / 2);

    reader1.add_samples(BufSz, 0.33f);
    reader2.add_samples(BufSz, 0.33f);
    expect_output(mixer, BufSz, 0.33f * 2, 0,
                  (start_ts1 + core::Second * 2) / 2
                      + (start_ts2 + core::Second * 2) / 2);

    CHECK(reader1.num_unread() == 0);
    CHECK(reader2.num_unread() == 0);
}

TEST(mixer, timestamps_disabled) {
    const SampleSpec sample_spec(BufSz, ChanLayout_Surround, ChanOrder_Smpte,
                                 ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts = 1000000000000;

    test::MockReader reader1;
    test::MockReader reader2;

    Mixer mixer(buffer_factory, false);
    CHECK(mixer.is_valid());

    reader1.enable_timestamps(start_ts, sample_spec);
    reader2.enable_timestamps(start_ts, sample_spec);

    mixer.add_input(reader1);

    reader1.add_samples(BufSz, 0.11f);
    expect_output(mixer, BufSz, 0.11f, 0, 0);

    mixer.add_input(reader2);

    reader1.add_samples(BufSz, 0.22f);
    reader2.add_samples(BufSz, 0.22f);
    expect_output(mixer, BufSz, 0.44f, 0, 0);

    CHECK(reader1.num_unread() == 0);
    CHECK(reader2.num_unread() == 0);
}

} // namespace audio
} // namespace roc
