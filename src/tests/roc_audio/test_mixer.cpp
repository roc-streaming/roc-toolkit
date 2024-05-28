/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_reader.h"

#include "roc_audio/mixer.h"
#include "roc_core/heap_arena.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

enum { BufSz = 100, MaxBufSz = 500, SampleRate = 44100 };

const SampleSpec sample_spec(SampleRate,
                             Sample_RawFormat,
                             ChanLayout_Surround,
                             ChanOrder_Smpte,
                             ChanMask_Surround_Mono);

core::HeapArena arena;
FrameFactory frame_factory(arena, MaxBufSz * sizeof(sample_t));
FrameFactory big_frame_factory(arena, MaxBufSz * 10 * sizeof(sample_t));

void expect_output(Mixer& mixer,
                   size_t samples_per_chan,
                   sample_t value,
                   unsigned flags = 0,
                   core::nanoseconds_t capture_ts = -1) {
    CHECK(sample_spec.num_channels() == 1);

    FramePtr frame = big_frame_factory.allocate_frame(0);
    CHECK(frame);

    LONGS_EQUAL(status::StatusOK, mixer.read(*frame, samples_per_chan));

    CHECK(frame->is_raw());

    UNSIGNED_LONGS_EQUAL(samples_per_chan, frame->num_raw_samples());
    UNSIGNED_LONGS_EQUAL(samples_per_chan, frame->duration());

    for (size_t n = 0; n < samples_per_chan; n++) {
        DOUBLES_EQUAL((double)value, (double)frame->raw_samples()[n], 0.0001);
    }

    UNSIGNED_LONGS_EQUAL(flags, frame->flags());

    if (capture_ts < 0) {
        LONGS_EQUAL(0, frame->capture_timestamp());
    } else {
        CHECK(core::ns_equal_delta(frame->capture_timestamp(), capture_ts,
                                   core::Microsecond));
    }
}

void expect_error(Mixer& mixer, size_t samples_per_chan, status::StatusCode status) {
    CHECK(sample_spec.num_channels() == 1);

    FramePtr frame = big_frame_factory.allocate_frame(0);
    CHECK(frame);

    LONGS_EQUAL(status, mixer.read(*frame, samples_per_chan));
}

} // namespace

TEST_GROUP(mixer) {};

TEST(mixer, no_readers) {
    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    expect_output(mixer, BufSz, 0);
}

TEST(mixer, one_reader) {
    test::MockReader reader(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    mixer.add_input(reader);

    reader.add_samples(BufSz, 0.11f);
    expect_output(mixer, BufSz, 0.11f);

    LONGS_EQUAL(0, reader.num_unread());
    LONGS_EQUAL(1, reader.total_reads());
}

TEST(mixer, one_reader_big_frame) {
    enum { Factor = 3 };

    test::MockReader reader(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    mixer.add_input(reader);

    reader.add_samples(MaxBufSz * Factor, 0.11f);
    expect_output(mixer, MaxBufSz * Factor, 0.11f);

    LONGS_EQUAL(0, reader.num_unread());
    LONGS_EQUAL(Factor, reader.total_reads());
}

TEST(mixer, two_readers) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.22f);

    expect_output(mixer, BufSz, 0.33f);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(1, reader1.total_reads());
    LONGS_EQUAL(1, reader2.total_reads());
}

TEST(mixer, remove_reader) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

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

    LONGS_EQUAL(BufSz, reader1.num_unread());
    LONGS_EQUAL(BufSz * 2, reader2.num_unread());
}

TEST(mixer, clamp) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

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

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());
}

TEST(mixer, flags) {
    enum { BigBatch = MaxBufSz * 2 };

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.add_samples(BigBatch, 0.1f, 0);
    reader1.add_samples(BigBatch, 0.1f, Frame::HasSignal);
    reader1.add_samples(BigBatch, 0.1f, 0);

    reader2.add_samples(BigBatch, 0.1f, Frame::HasHoles);
    reader2.add_samples(BigBatch / 2, 0.1f, 0);
    reader2.add_samples(BigBatch / 2, 0.1f, Frame::HasPacketDrops);
    reader2.add_samples(BigBatch, 0.1f, 0);

    expect_output(mixer, BigBatch, 0.2f, Frame::HasHoles);
    expect_output(mixer, BigBatch, 0.2f, Frame::HasSignal | Frame::HasPacketDrops);
    expect_output(mixer, BigBatch, 0.2f, 0);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());
}

TEST(mixer, timestamps_one_reader) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, Sample_RawFormat, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts = 1000000000000;

    test::MockReader reader(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    mixer.add_input(reader);

    reader.enable_timestamps(start_ts);

    reader.add_samples(BufSz, 0.11f);
    expect_output(mixer, BufSz, 0.11f, 0, start_ts);

    reader.add_samples(BufSz, 0.22f);
    expect_output(mixer, BufSz, 0.22f, 0, start_ts + core::Second);

    reader.add_samples(BufSz, 0.33f);
    expect_output(mixer, BufSz, 0.33f, 0, start_ts + core::Second * 2);

    LONGS_EQUAL(0, reader.num_unread());
}

TEST(mixer, timestamps_two_readers) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, Sample_RawFormat, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts1 = 2000000000000;
    const core::nanoseconds_t start_ts2 = 1000000000000;

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.enable_timestamps(start_ts1);
    reader2.enable_timestamps(start_ts2);

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

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());
}

TEST(mixer, timestamps_partial) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, Sample_RawFormat, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts1 = 2000000000000;
    const core::nanoseconds_t start_ts2 = 1000000000000;

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);
    test::MockReader reader3(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    mixer.add_input(reader1);
    mixer.add_input(reader2);
    mixer.add_input(reader3);

    reader1.enable_timestamps(start_ts1);
    reader2.enable_timestamps(start_ts2);
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

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());
    CHECK(reader3.num_unread() == 0);
}

TEST(mixer, timestamps_overflow_handling) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, Sample_RawFormat, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts1 = 9000000000000000000ll;
    const core::nanoseconds_t start_ts2 = 9100000000000000000ll;

    // ensure there would be an overflow if we directly sum timestamps
    // mixer should produce correct results despite of that
    CHECK(int64_t(uint64_t(start_ts1) + uint64_t(start_ts2)) < 0);

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.enable_timestamps(start_ts1);
    reader2.enable_timestamps(start_ts2);

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

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());
}

TEST(mixer, timestamps_disabled) {
    const SampleSpec sample_spec(BufSz, Sample_RawFormat, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts = 1000000000000;

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, false);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    reader1.enable_timestamps(start_ts);
    reader2.enable_timestamps(start_ts);

    mixer.add_input(reader1);

    reader1.add_samples(BufSz, 0.11f);
    expect_output(mixer, BufSz, 0.11f, 0, 0);

    mixer.add_input(reader2);

    reader1.add_samples(BufSz, 0.22f);
    reader2.add_samples(BufSz, 0.22f);
    expect_output(mixer, BufSz, 0.44f, 0, 0);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());
}

// If input reader returns StatusDrain, mixer skips it.
TEST(mixer, process_drained) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.add_samples(BufSz * 4, 0.11f);
    reader2.add_samples(BufSz * 4, 0.22f);

    reader1.set_status(status::StatusOK);
    reader2.set_status(status::StatusOK);

    expect_output(mixer, BufSz, 0.33f, 0, 0);

    reader1.set_status(status::StatusDrain);
    reader2.set_status(status::StatusOK);

    expect_output(mixer, BufSz, 0.22f, 0, 0);

    reader1.set_status(status::StatusOK);
    reader2.set_status(status::StatusDrain);

    expect_output(mixer, BufSz, 0.11f, 0, 0);

    reader1.set_status(status::StatusOK);
    reader2.set_status(status::StatusOK);

    expect_output(mixer, BufSz, 0.33f, 0, 0);
    expect_output(mixer, BufSz, 0.33f, 0, 0);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(4, reader1.total_reads());
    LONGS_EQUAL(4, reader2.total_reads());
}

// If input reader returns StatusPart, mixer repeats read
// until it gathers complete frame.
TEST(mixer, process_partial) {
    enum { Factor1 = 2, Factor2 = 4 };

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.add_samples(BufSz * 2, 0.11f);
    reader2.add_samples(BufSz * 2, 0.22f);

    reader1.set_limit(BufSz / Factor1);
    reader2.set_limit(BufSz / Factor2);

    expect_output(mixer, BufSz, 0.33f, 0, 0);

    LONGS_EQUAL(BufSz, reader1.num_unread());
    LONGS_EQUAL(BufSz, reader2.num_unread());

    LONGS_EQUAL(Factor1, reader1.total_reads());
    LONGS_EQUAL(Factor2, reader2.total_reads());

    reader1.set_limit(BufSz / Factor2);
    reader2.set_limit(BufSz / Factor1);

    expect_output(mixer, BufSz, 0.33f, 0, 0);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(Factor1 + Factor2, reader1.total_reads());
    LONGS_EQUAL(Factor1 + Factor2, reader2.total_reads());
}

// If any of input readers returns error, mixer forwards it.
TEST(mixer, forward_error) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(frame_factory, sample_spec, true);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    mixer.add_input(reader1);
    mixer.add_input(reader2);

    reader1.add_zero_samples();
    reader2.add_zero_samples();

    reader1.set_status(status::StatusAbort);
    reader2.set_status(status::StatusOK);

    expect_error(mixer, BufSz, status::StatusAbort);

    reader1.set_status(status::StatusOK);
    reader2.set_status(status::StatusAbort);

    expect_error(mixer, BufSz, status::StatusAbort);

    reader1.set_status(status::StatusOK);
    reader2.set_status(status::StatusOK);

    expect_output(mixer, BufSz, 0, 0, 0);
}

// Attach to frame pre-allocated buffers of different sizes before reading.
TEST(mixer, preallocated_buffer) {
    const size_t buffer_list[] = {
        BufSz * 50, // big size (reader should use it)
        BufSz,      // exact size (reader should use it)
        BufSz - 1,  // small size (reader should replace buffer)
        0,          // no buffer (reader should allocate buffer)
    };

    for (size_t bn = 0; bn < ROC_ARRAY_SIZE(buffer_list); bn++) {
        const size_t orig_buf_sz = buffer_list[bn];

        test::MockReader reader1(frame_factory, sample_spec);
        test::MockReader reader2(frame_factory, sample_spec);

        Mixer mixer(frame_factory, sample_spec, true);
        LONGS_EQUAL(status::StatusOK, mixer.init_status());

        reader1.add_zero_samples();
        reader2.add_zero_samples();

        mixer.add_input(reader1);
        mixer.add_input(reader2);

        FrameFactory mock_factory(arena, orig_buf_sz * sizeof(sample_t));
        FramePtr frame = orig_buf_sz > 0 ? mock_factory.allocate_frame(0)
                                         : mock_factory.allocate_frame_no_buffer();

        core::Slice<uint8_t> orig_buf = frame->buffer();

        LONGS_EQUAL(status::StatusOK, mixer.read(*frame, BufSz));

        CHECK(frame->buffer());

        if (orig_buf_sz >= BufSz) {
            CHECK(frame->buffer() == orig_buf);
        } else {
            CHECK(frame->buffer() != orig_buf);
        }

        LONGS_EQUAL(BufSz, frame->duration());
        LONGS_EQUAL(BufSz, frame->num_raw_samples());
        LONGS_EQUAL(BufSz * sizeof(sample_t), frame->num_bytes());
    }
}

} // namespace audio
} // namespace roc
