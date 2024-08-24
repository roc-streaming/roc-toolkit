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
                             PcmSubformat_Raw,
                             ChanLayout_Surround,
                             ChanOrder_Smpte,
                             ChanMask_Surround_Mono);

core::HeapArena arena;
FrameFactory frame_factory(arena, MaxBufSz * sizeof(sample_t));
FrameFactory big_frame_factory(arena, MaxBufSz * 10 * sizeof(sample_t));

void expect_output(status::StatusCode expected_code,
                   Mixer& mixer,
                   size_t requested_samples,
                   size_t expected_samples,
                   sample_t value,
                   unsigned flags = 0,
                   core::nanoseconds_t capture_ts = -1,
                   FrameReadMode mode = ModeHard) {
    CHECK(sample_spec.num_channels() == 1);

    FramePtr frame = big_frame_factory.allocate_frame(0);
    CHECK(frame);

    LONGS_EQUAL(expected_code, mixer.read(*frame, requested_samples, mode));

    CHECK(frame->is_raw());

    UNSIGNED_LONGS_EQUAL(expected_samples, frame->num_raw_samples());
    UNSIGNED_LONGS_EQUAL(expected_samples, frame->duration());

    for (size_t n = 0; n < expected_samples; n++) {
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

void expect_error(status::StatusCode expected_code,
                  Mixer& mixer,
                  size_t requested_samples,
                  FrameReadMode mode = ModeHard) {
    CHECK(sample_spec.num_channels() == 1);

    FramePtr frame = big_frame_factory.allocate_frame(0);
    CHECK(frame);

    LONGS_EQUAL(expected_code, mixer.read(*frame, requested_samples, mode));
}

} // namespace

TEST_GROUP(mixer) {};

TEST(mixer, no_readers) {
    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0);
}

TEST(mixer, one_input) {
    test::MockReader reader(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader));

    reader.add_samples(BufSz, 0.11f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.11f);

    LONGS_EQUAL(0, reader.num_unread());
    LONGS_EQUAL(1, reader.total_reads());
}

TEST(mixer, one_input_big_frame) {
    enum { Factor = 3 };

    test::MockReader reader(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader));

    reader.add_samples(MaxBufSz * Factor, 0.11f);
    expect_output(status::StatusOK, mixer, MaxBufSz * Factor, MaxBufSz * Factor, 0.11f);

    LONGS_EQUAL(0, reader.num_unread());
    LONGS_EQUAL(Factor, reader.total_reads());
}

TEST(mixer, two_inputs) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.22f);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(1, reader1.total_reads());
    LONGS_EQUAL(1, reader2.total_reads());
}

TEST(mixer, remove_input) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.22f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f);

    mixer.remove_input(reader2);

    reader1.add_samples(BufSz, 0.44f);
    reader2.add_samples(BufSz, 0.55f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.44f);

    mixer.remove_input(reader1);

    reader1.add_samples(BufSz, 0.77f);
    reader2.add_samples(BufSz, 0.88f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.0f);

    LONGS_EQUAL(BufSz, reader1.num_unread());
    LONGS_EQUAL(BufSz * 2, reader2.num_unread());
}

TEST(mixer, has_input) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    CHECK(!mixer.has_input(reader1));
    CHECK(!mixer.has_input(reader2));

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    CHECK(mixer.has_input(reader1));
    CHECK(!mixer.has_input(reader2));

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));
    CHECK(mixer.has_input(reader1));
    CHECK(mixer.has_input(reader2));

    mixer.remove_input(reader1);
    CHECK(!mixer.has_input(reader1));
    CHECK(mixer.has_input(reader2));

    mixer.remove_input(reader2);
    CHECK(!mixer.has_input(reader1));
    CHECK(!mixer.has_input(reader2));
}

// If reader returns StatusFinish, mixer skips it.
TEST(mixer, finish) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.22f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f);

    reader2.set_status(status::StatusFinish);

    reader1.add_samples(BufSz, 0.44f);
    reader2.add_samples(BufSz, 0.55f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.44f);

    reader1.set_status(status::StatusFinish);

    reader1.add_samples(BufSz, 0.77f);
    reader2.add_samples(BufSz, 0.88f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.0f);

    LONGS_EQUAL(BufSz, reader1.num_unread());
    LONGS_EQUAL(BufSz * 2, reader2.num_unread());
}

// If input reader returns StatusPart, mixer repeats read
// until it gathers complete frame.
TEST(mixer, partial) {
    enum { Factor1 = 2, Factor2 = 4 };

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    reader1.add_samples(BufSz * 2, 0.11f);
    reader2.add_samples(BufSz * 2, 0.22f);

    reader1.set_limit(BufSz / Factor1);
    reader2.set_limit(BufSz / Factor2);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f, 0, 0);

    LONGS_EQUAL(BufSz, reader1.num_unread());
    LONGS_EQUAL(BufSz, reader2.num_unread());

    LONGS_EQUAL(Factor1, reader1.total_reads());
    LONGS_EQUAL(Factor2, reader2.total_reads());

    reader1.set_limit(BufSz / Factor2);
    reader2.set_limit(BufSz / Factor1);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f, 0, 0);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(Factor1 + Factor2, reader1.total_reads());
    LONGS_EQUAL(Factor1 + Factor2, reader2.total_reads());
}

// Reader returns StatusFinish in the middle of repeating partial.
TEST(mixer, partial_end) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    reader1.add_samples(BufSz * 2, 0.11f);
    reader2.add_samples(BufSz * 2, 0.22f);
    reader2.add_samples(BufSz * 2, 0.33f);

    reader1.set_limit(BufSz);
    reader2.set_limit(BufSz);

    reader1.set_no_samples_status(status::StatusFinish);

    expect_output(status::StatusOK, mixer, BufSz * 4, BufSz * 4, 0.33f, 0, 0);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(3, reader1.total_reads());
    LONGS_EQUAL(4, reader2.total_reads());

    LONGS_EQUAL(status::StatusFinish, reader1.last_status());
    LONGS_EQUAL(status::StatusOK, reader2.last_status());
}

TEST(mixer, clamp) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    reader1.add_samples(BufSz, 0.900f);
    reader2.add_samples(BufSz, 0.101f);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 1.0f);

    reader1.add_samples(BufSz, 0.2f);
    reader2.add_samples(BufSz, 1.1f);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 1.0f);

    reader1.add_samples(BufSz, -0.2f);
    reader2.add_samples(BufSz, -0.81f);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, -1.0f);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());
}

TEST(mixer, cts_one_reader) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts = 1000000000000;

    test::MockReader reader(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader));

    reader.enable_timestamps(start_ts);

    reader.add_samples(BufSz, 0.11f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.11f, 0, start_ts);

    reader.add_samples(BufSz, 0.22f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.22f, 0,
                  start_ts + core::Second);

    reader.add_samples(BufSz, 0.33f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f, 0,
                  start_ts + core::Second * 2);

    LONGS_EQUAL(0, reader.num_unread());
}

TEST(mixer, cts_two_readers) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts1 = 2000000000000;
    const core::nanoseconds_t start_ts2 = 1000000000000;

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    reader1.enable_timestamps(start_ts1);
    reader2.enable_timestamps(start_ts2);

    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.11f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.11f * 2, 0,
                  (start_ts1 + start_ts2) / 2);

    reader1.add_samples(BufSz, 0.22f);
    reader2.add_samples(BufSz, 0.22f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.22f * 2, 0,
                  ((start_ts1 + core::Second) + (start_ts2 + core::Second)) / 2);

    reader1.add_samples(BufSz, 0.33f);
    reader2.add_samples(BufSz, 0.33f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f * 2, 0,
                  ((start_ts1 + core::Second * 2) + (start_ts2 + core::Second * 2)) / 2);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());
}

TEST(mixer, cts_partial) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts1 = 2000000000000;
    const core::nanoseconds_t start_ts2 = 1000000000000;

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);
    test::MockReader reader3(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader3));

    reader1.enable_timestamps(start_ts1);
    reader2.enable_timestamps(start_ts2);
    // reader3 does not have timestamps

    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.11f);
    reader3.add_samples(BufSz, 0.11f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.11f * 3, 0,
                  (start_ts1 + start_ts2) / 3);

    reader1.add_samples(BufSz, 0.22f);
    reader2.add_samples(BufSz, 0.22f);
    reader3.add_samples(BufSz, 0.22f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.22f * 3, 0,
                  ((start_ts1 + core::Second) + (start_ts2 + core::Second)) / 3);

    reader1.add_samples(BufSz, 0.33f);
    reader2.add_samples(BufSz, 0.33f);
    reader3.add_samples(BufSz, 0.33f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f * 3, 0,
                  ((start_ts1 + core::Second * 2) + (start_ts2 + core::Second * 2)) / 3);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());
    LONGS_EQUAL(0, reader3.num_unread());
}

TEST(mixer, cts_prevent_overflow) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts1 = 9000000000000000000ll;
    const core::nanoseconds_t start_ts2 = 9100000000000000000ll;

    // ensure there would be an overflow if we directly sum timestamps
    // mixer should produce correct results despite of that
    CHECK(int64_t(uint64_t(start_ts1) + uint64_t(start_ts2)) < 0);

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    reader1.enable_timestamps(start_ts1);
    reader2.enable_timestamps(start_ts2);

    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.11f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.11f * 2, 0,
                  start_ts1 / 2 + start_ts2 / 2);

    reader1.add_samples(BufSz, 0.22f);
    reader2.add_samples(BufSz, 0.22f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.22f * 2, 0,
                  (start_ts1 + core::Second) / 2 + (start_ts2 + core::Second) / 2);

    reader1.add_samples(BufSz, 0.33f);
    reader2.add_samples(BufSz, 0.33f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f * 2, 0,
                  (start_ts1 + core::Second * 2) / 2
                      + (start_ts2 + core::Second * 2) / 2);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());
}

TEST(mixer, cts_disabled) {
    const SampleSpec sample_spec(BufSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts = 1000000000000;

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, false, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    reader1.enable_timestamps(start_ts);
    reader2.enable_timestamps(start_ts);

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));

    reader1.add_samples(BufSz, 0.11f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.11f, 0, 0);

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    reader1.add_samples(BufSz, 0.22f);
    reader2.add_samples(BufSz, 0.22f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.44f, 0, 0);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());
}

// In soft read mode, input returns StatusDrain.
TEST(mixer, soft_read_drain) {
    enum { Factor1 = 2, Factor2 = 4 };

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    // both readers return StatusDrain
    expect_error(status::StatusDrain, mixer, BufSz, ModeSoft);

    LONGS_EQUAL(1, reader1.total_reads());
    LONGS_EQUAL(1, reader2.total_reads());

    // reader1 returns StatusDrain
    // reader2 returns StatusOK
    reader2.add_samples(BufSz, 0.11f);

    expect_error(status::StatusDrain, mixer, BufSz, ModeSoft);

    LONGS_EQUAL(2, reader1.total_reads());
    LONGS_EQUAL(2, reader2.total_reads());

    // reader1 returns StatusOK
    reader1.add_samples(BufSz, 0.22f);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f, 0, 0, ModeSoft);

    LONGS_EQUAL(3, reader1.total_reads());
    LONGS_EQUAL(2, reader2.total_reads());
}

// In soft read mode, if input reader returns StatusPartial,
// mixer repeats read.
TEST(mixer, soft_read_partial) {
    enum { Factor1 = 2, Factor2 = 4 };

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    reader1.add_samples(BufSz * 2, 0.11f);
    reader2.add_samples(BufSz * 2, 0.22f);

    reader1.set_limit(BufSz / Factor1);
    reader2.set_limit(BufSz / Factor2);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f, 0, 0, ModeSoft);

    LONGS_EQUAL(BufSz, reader1.num_unread());
    LONGS_EQUAL(BufSz, reader2.num_unread());

    LONGS_EQUAL(Factor1, reader1.total_reads());
    LONGS_EQUAL(Factor2, reader2.total_reads());

    reader1.set_limit(BufSz / Factor2);
    reader2.set_limit(BufSz / Factor1);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f, 0, 0, ModeSoft);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(Factor1 + Factor2, reader1.total_reads());
    LONGS_EQUAL(Factor1 + Factor2, reader2.total_reads());
}

// In soft read mode, if input reader returns StatusDrain,
// mixer generates partial read.
TEST(mixer, soft_read_partial_drain) {
    test::MockReader reader(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader));

    // mock reader returns StatusPart, then StatusDrain
    reader.add_samples(BufSz, 0.11f);
    expect_output(status::StatusPart, mixer, BufSz * 2, BufSz, 0.11f, 0, 0, ModeSoft);

    LONGS_EQUAL(0, reader.num_unread());
    LONGS_EQUAL(2, reader.total_reads());

    LONGS_EQUAL(status::StatusDrain, reader.last_status());

    // mock reader returns StatusDrain
    expect_error(status::StatusDrain, mixer, BufSz, ModeSoft);

    LONGS_EQUAL(0, reader.num_unread());
    LONGS_EQUAL(3, reader.total_reads());

    LONGS_EQUAL(status::StatusDrain, reader.last_status());

    // mock reader returns StatusOK
    reader.add_samples(BufSz, 0.22f);
    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.22f, 0, 0, ModeSoft);

    LONGS_EQUAL(0, reader.num_unread());
    LONGS_EQUAL(4, reader.total_reads());

    LONGS_EQUAL(status::StatusOK, reader.last_status());
}

// Same as above, but there are two readers.
TEST(mixer, soft_read_partial_drain_two_readers) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    // reader1 returns StatusOK
    // reader2 returns StatusPart, then StatusDrain
    reader1.add_samples(BufSz * 2, 0.11f);
    reader2.add_samples(BufSz, 0.22f);

    expect_output(status::StatusPart, mixer, BufSz * 2, BufSz, 0.33f, 0, 0, ModeSoft);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(1, reader1.total_reads());
    LONGS_EQUAL(2, reader2.total_reads());

    LONGS_EQUAL(status::StatusOK, reader1.last_status());
    LONGS_EQUAL(status::StatusDrain, reader2.last_status());

    // reader2 returns StatusDrain
    expect_error(status::StatusDrain, mixer, BufSz, ModeSoft);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(1, reader1.total_reads());
    LONGS_EQUAL(3, reader2.total_reads());

    LONGS_EQUAL(status::NoStatus, reader1.last_status());
    LONGS_EQUAL(status::StatusDrain, reader2.last_status());

    // reader1 returns StatusOK
    // reader2 returns StatusDrain
    reader1.add_samples(BufSz, 0.11f);

    expect_error(status::StatusDrain, mixer, BufSz * 2, ModeSoft);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(2, reader1.total_reads());
    LONGS_EQUAL(4, reader2.total_reads());

    LONGS_EQUAL(status::StatusOK, reader1.last_status());
    LONGS_EQUAL(status::StatusDrain, reader2.last_status());

    // reader2 returns StatusOK
    reader2.add_samples(BufSz * 2, 0.22f);

    expect_output(status::StatusOK, mixer, BufSz * 2, BufSz * 2, 0.33f, 0, 0, ModeSoft);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(2, reader1.total_reads());
    LONGS_EQUAL(5, reader2.total_reads());

    LONGS_EQUAL(status::NoStatus, reader1.last_status());
    LONGS_EQUAL(status::StatusOK, reader2.last_status());
}

// One reader returns StatusFinish during soft read.
TEST(mixer, soft_read_partial_end_two_readers) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    // reader1 returns StatusOK
    // reader2 returns StatusPart, then StatusFinish
    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz, 0.22f);
    reader2.add_samples(BufSz, 0.33f);

    reader1.set_no_samples_status(status::StatusFinish);

    reader1.set_limit(BufSz / 2);
    reader2.set_limit(BufSz / 2);

    expect_output(status::StatusOK, mixer, BufSz * 2, BufSz * 2, 0.33f, 0, 0, ModeSoft);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(3, reader1.total_reads());
    LONGS_EQUAL(4, reader2.total_reads());

    LONGS_EQUAL(status::StatusFinish, reader1.last_status());
    LONGS_EQUAL(status::StatusOK, reader2.last_status());
}

// Soft reads and capture timestamps.
TEST(mixer, soft_read_cts) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts = 1000000000000;

    test::MockReader reader(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader));

    reader.enable_timestamps(start_ts);

    reader.add_samples(BufSz, 0.11f);
    expect_output(status::StatusPart, mixer, BufSz * 2, BufSz, 0.11f, 0, start_ts,
                  ModeSoft);

    LONGS_EQUAL(0, reader.num_unread());
    LONGS_EQUAL(2, reader.total_reads());

    reader.add_samples(BufSz, 0.22f);
    expect_output(status::StatusPart, mixer, BufSz * 2, BufSz, 0.22f, 0,
                  start_ts + core::Second, ModeSoft);

    LONGS_EQUAL(0, reader.num_unread());
    LONGS_EQUAL(4, reader.total_reads());
}

// Soft reads and capture timestamps.
TEST(mixer, soft_read_cts_partial) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts = 1000000000000;

    test::MockReader reader(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader));

    reader.enable_timestamps(start_ts);
    reader.set_limit(BufSz / 2);

    reader.add_samples(BufSz, 0.11f);
    expect_output(status::StatusPart, mixer, BufSz * 2, BufSz, 0.11f, 0, start_ts,
                  ModeSoft);

    LONGS_EQUAL(0, reader.num_unread());
    LONGS_EQUAL(3, reader.total_reads());

    reader.add_samples(BufSz, 0.22f);
    expect_output(status::StatusPart, mixer, BufSz * 2, BufSz, 0.22f, 0,
                  start_ts + core::Second, ModeSoft);

    LONGS_EQUAL(0, reader.num_unread());
    LONGS_EQUAL(6, reader.total_reads());
}

// Same as above, but there are two readers, and one returns StatusDrain.
TEST(mixer, soft_read_cts_two_readers) {
    // BufSz samples per second
    const SampleSpec sample_spec(BufSz, PcmSubformat_Raw, ChanLayout_Surround,
                                 ChanOrder_Smpte, ChanMask_Surround_Mono);
    const core::nanoseconds_t start_ts1 = 2000000000000;
    const core::nanoseconds_t start_ts2 = 1000000000000;

    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    reader1.enable_timestamps(start_ts1);
    reader2.enable_timestamps(start_ts2);

    // reader1 returns StatusDrain
    // output from reader2 is buffered
    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz * 2, 0.22f);

    expect_output(status::StatusPart, mixer, BufSz * 2, BufSz, 0.33f, 0,
                  (start_ts1 + start_ts2) / 2, ModeSoft);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(2, reader1.total_reads());
    LONGS_EQUAL(1, reader2.total_reads());

    // reader1 returns StatusOK
    // CTS for buffered output of reader2 is interpolated
    reader1.add_samples(BufSz, 0.11f);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.33f, 0,
                  (start_ts1 + start_ts2) / 2 + core::Second, ModeSoft);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(3, reader1.total_reads());
    LONGS_EQUAL(1, reader2.total_reads());
}

// Add new reader when there are buffered samples from a soft read.
TEST(mixer, soft_read_add_reader) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);
    test::MockReader reader3(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    // reader1 returns StatusDrain
    // output from reader2 is buffered
    reader1.add_samples(BufSz, 0.11f);
    reader2.add_samples(BufSz * 2, 0.22f);

    expect_output(status::StatusPart, mixer, BufSz * 2, BufSz, 0.33f, 0, 0, ModeSoft);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());

    LONGS_EQUAL(2, reader1.total_reads());
    LONGS_EQUAL(1, reader2.total_reads());

    // add reader3
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader3));

    // reader1 and reader3 return StatusOK
    reader1.add_samples(BufSz, 0.11f);
    reader3.add_samples(BufSz, 0.33f);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.66f, 0, 0, ModeSoft);

    LONGS_EQUAL(0, reader1.num_unread());
    LONGS_EQUAL(0, reader2.num_unread());
    LONGS_EQUAL(0, reader3.num_unread());

    LONGS_EQUAL(3, reader1.total_reads());
    LONGS_EQUAL(1, reader2.total_reads());
    LONGS_EQUAL(1, reader3.total_reads());
}

// Remove reader when there are buffered samples from a soft read.
TEST(mixer, soft_read_remove_reader) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);
    test::MockReader reader3(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader3));

    // mixer buffer after this read:
    //  reader1: 0
    //  reader2: BufSz*2
    //  reader3: BufSz*3
    reader1.add_samples(BufSz * 1, 0.11f);
    reader2.add_samples(BufSz * 3, 0.22f);
    reader3.add_samples(BufSz * 4, 0.33f);

    expect_output(status::StatusPart, mixer, BufSz * 4, BufSz, 0.66f, 0, 0, ModeSoft);

    LONGS_EQUAL(2, reader1.total_reads());
    LONGS_EQUAL(2, reader2.total_reads());
    LONGS_EQUAL(1, reader3.total_reads());

    // remove reader3
    mixer.remove_input(reader3);

    // mixer buffer after this read:
    //  reader1: 0
    //  reader2: BufSz
    //  reader3: BufSz (part beyond reader2 zeroized)
    reader1.add_samples(BufSz, 0.11f);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.66f, 0, 0, ModeSoft);

    LONGS_EQUAL(3, reader1.total_reads());
    LONGS_EQUAL(2, reader2.total_reads());
    LONGS_EQUAL(1, reader3.total_reads());

    // mixer buffer after this read:
    //  reader1: 0
    //  reader2: 0
    //  reader3: 0
    reader1.add_samples(BufSz, 0.11f);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.66f, 0, 0, ModeSoft);

    LONGS_EQUAL(4, reader1.total_reads());
    LONGS_EQUAL(2, reader2.total_reads());
    LONGS_EQUAL(1, reader3.total_reads());

    // mixer buffer after this read:
    //  reader1: 0
    //  reader2: 0
    //  reader3: 0
    reader1.add_samples(BufSz * 4, 0.11f);
    reader2.add_samples(BufSz * 4, 0.22f);

    expect_output(status::StatusOK, mixer, BufSz * 4, BufSz * 4, 0.33f, 0, 0, ModeSoft);

    LONGS_EQUAL(5, reader1.total_reads());
    LONGS_EQUAL(3, reader2.total_reads());
    LONGS_EQUAL(1, reader3.total_reads());

    // remove reader2
    mixer.remove_input(reader2);

    // mixer buffer after this read:
    //  reader1: 0
    //  reader2: 0
    //  reader3: 0
    reader1.add_samples(BufSz * 4, 0.11f);

    expect_output(status::StatusOK, mixer, BufSz * 4, BufSz * 4, 0.11f, 0, 0, ModeSoft);

    LONGS_EQUAL(6, reader1.total_reads());
    LONGS_EQUAL(3, reader2.total_reads());
    LONGS_EQUAL(1, reader3.total_reads());
}

// Mixer forwards reading mode to underlying reader.
TEST(mixer, forward_mode) {
    test::MockReader reader(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader));

    reader.add_zero_samples();

    const FrameReadMode mode_list[] = {
        ModeHard,
        ModeSoft,
    };

    for (size_t md_n = 0; md_n < ROC_ARRAY_SIZE(mode_list); md_n++) {
        expect_output(status::StatusOK, mixer, BufSz, BufSz, 0.00f, 0, 0,
                      mode_list[md_n]);

        LONGS_EQUAL(mode_list[md_n], reader.last_mode());
    }
}

// If any of input readers returns error, mixer forwards it.
TEST(mixer, forward_error) {
    test::MockReader reader1(frame_factory, sample_spec);
    test::MockReader reader2(frame_factory, sample_spec);

    Mixer mixer(sample_spec, true, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, mixer.init_status());

    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
    LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

    reader1.add_zero_samples();
    reader2.add_zero_samples();

    // reader1 fails
    reader1.set_status(status::StatusAbort);
    reader2.set_status(status::StatusOK);

    expect_error(status::StatusAbort, mixer, BufSz);

    LONGS_EQUAL(1, reader1.total_reads());
    LONGS_EQUAL(0, reader2.total_reads());

    // reader2 fails
    reader1.set_status(status::StatusOK);
    reader2.set_status(status::StatusAbort);

    expect_error(status::StatusAbort, mixer, BufSz);

    LONGS_EQUAL(2, reader1.total_reads());
    LONGS_EQUAL(1, reader2.total_reads());

    // both readers work
    reader1.set_status(status::StatusOK);
    reader2.set_status(status::StatusOK);

    expect_output(status::StatusOK, mixer, BufSz, BufSz, 0, 0, 0);

    LONGS_EQUAL(2, reader1.total_reads());
    LONGS_EQUAL(2, reader2.total_reads());
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

        Mixer mixer(sample_spec, true, frame_factory, arena);
        LONGS_EQUAL(status::StatusOK, mixer.init_status());

        reader1.add_zero_samples();
        reader2.add_zero_samples();

        LONGS_EQUAL(status::StatusOK, mixer.add_input(reader1));
        LONGS_EQUAL(status::StatusOK, mixer.add_input(reader2));

        FrameFactory mock_factory(arena, orig_buf_sz * sizeof(sample_t));
        FramePtr frame = orig_buf_sz > 0 ? mock_factory.allocate_frame(0)
                                         : mock_factory.allocate_frame_no_buffer();

        core::Slice<uint8_t> orig_buf = frame->buffer();

        LONGS_EQUAL(status::StatusOK, mixer.read(*frame, BufSz, ModeHard));

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
