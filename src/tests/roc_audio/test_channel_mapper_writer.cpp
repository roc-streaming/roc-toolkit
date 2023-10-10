/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_writer.h"

#include "roc_audio/channel_mapper_writer.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

namespace {

const double Epsilon = 0.00001;

enum { MaxSz = 500 };

core::HeapArena arena;
core::BufferFactory<sample_t> buffer_factory(arena, MaxSz);

void fill_mono(Frame& frame, sample_t value) {
    CHECK(frame.num_samples() > 0);

    for (size_t n = 0; n < frame.num_samples(); n++) {
        frame.samples()[n] = value;
    }
}

void fill_stereo(Frame& frame, sample_t left_value, sample_t right_value) {
    CHECK(frame.num_samples() > 0);
    CHECK(frame.num_samples() % 2 == 0);

    for (size_t n = 0; n < frame.num_samples(); n += 2) {
        frame.samples()[n + 0] = left_value;
        frame.samples()[n + 1] = right_value;
    }
}

void expect_mono(test::MockWriter& mock_writer, size_t size, sample_t value) {
    CHECK(size > 0);
    CHECK(size <= mock_writer.num_unread());

    for (size_t n = 0; n < size; n++) {
        DOUBLES_EQUAL((double)value, (double)mock_writer.get(), Epsilon);
    }
}

void expect_stereo(test::MockWriter& mock_writer,
                   size_t size,
                   sample_t left_value,
                   sample_t right_value) {
    CHECK(size > 0);
    CHECK(size % 2 == 0);
    CHECK(size <= mock_writer.num_unread());

    for (size_t n = 0; n < size; n += 2) {
        DOUBLES_EQUAL((double)left_value, (double)mock_writer.get(), Epsilon);
        DOUBLES_EQUAL((double)right_value, (double)mock_writer.get(), Epsilon);
    }
}

} // namespace

TEST_GROUP(channel_mapper_writer) {};

TEST(channel_mapper_writer, small_frame_upmix) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(MaxSz, ChanLayout_Surround, ChanOrder_Smpte,
                              ChanMask_Surround_Stereo);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, buffer_factory, in_spec, out_spec);

    sample_t samples[FrameSz] = {};
    const unsigned flags = Frame::FlagIncomplete;
    const core::nanoseconds_t timestamp = 1000000;

    Frame frame(samples, FrameSz);
    frame.set_flags(flags);
    frame.set_capture_timestamp(timestamp);
    fill_mono(frame, 0.3f);

    mapper_writer.write(frame);

    CHECK_EQUAL(1, mock_writer.n_writes());

    CHECK_EQUAL(FrameSz * 2, mock_writer.frame_size(0));
    CHECK_EQUAL(flags, mock_writer.frame_flags(0));
    CHECK_EQUAL(timestamp, mock_writer.frame_timestamp(0));

    expect_stereo(mock_writer, FrameSz * 2, 0.3f, 0.3f);

    CHECK_EQUAL(0, mock_writer.num_unread());
}

TEST(channel_mapper_writer, small_frame_downmix) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Stereo);
    const SampleSpec out_spec(MaxSz, ChanLayout_Surround, ChanOrder_Smpte,
                              ChanMask_Surround_Mono);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, buffer_factory, in_spec, out_spec);

    sample_t samples[FrameSz] = {};
    const unsigned flags = Frame::FlagIncomplete;
    const core::nanoseconds_t timestamp = 1000000;

    Frame frame(samples, FrameSz);
    frame.set_flags(flags);
    frame.set_capture_timestamp(timestamp);
    fill_stereo(frame, 0.2f, 0.4f);

    mapper_writer.write(frame);

    CHECK_EQUAL(1, mock_writer.n_writes());

    CHECK_EQUAL(FrameSz / 2, mock_writer.frame_size(0));
    CHECK_EQUAL(flags, mock_writer.frame_flags(0));
    CHECK_EQUAL(timestamp, mock_writer.frame_timestamp(0));

    expect_mono(mock_writer, FrameSz / 2, 0.3f);

    CHECK_EQUAL(0, mock_writer.num_unread());
}

TEST(channel_mapper_writer, small_frame_nocts) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Stereo);
    const SampleSpec out_spec(MaxSz, ChanLayout_Surround, ChanOrder_Smpte,
                              ChanMask_Surround_Mono);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, buffer_factory, in_spec, out_spec);

    sample_t samples[FrameSz] = {};
    const unsigned flags = Frame::FlagIncomplete;

    Frame frame(samples, FrameSz);
    frame.set_flags(flags);
    frame.set_capture_timestamp(0);
    fill_stereo(frame, 0.2f, 0.4f);

    mapper_writer.write(frame);

    CHECK_EQUAL(1, mock_writer.n_writes());

    CHECK_EQUAL(FrameSz / 2, mock_writer.frame_size(0));
    CHECK_EQUAL(flags, mock_writer.frame_flags(0));
    CHECK_EQUAL(0, mock_writer.frame_timestamp(0));

    expect_mono(mock_writer, FrameSz / 2, 0.3f);

    CHECK_EQUAL(0, mock_writer.num_unread());
}

TEST(channel_mapper_writer, large_frame_upmix) {
    enum { FrameSz = MaxSz * 3 };

    const SampleSpec in_spec(MaxSz, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(MaxSz, ChanLayout_Surround, ChanOrder_Smpte,
                              ChanMask_Surround_Stereo);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, buffer_factory, in_spec, out_spec);

    sample_t samples[FrameSz] = {};
    const unsigned flags = Frame::FlagIncomplete;
    const core::nanoseconds_t timestamp = 1000000;

    Frame frame(samples, FrameSz);
    frame.set_flags(flags);
    frame.set_capture_timestamp(timestamp);
    fill_mono(frame, 0.3f);

    mapper_writer.write(frame);

    CHECK_EQUAL(6, mock_writer.n_writes());

    for (size_t i = 0; i < mock_writer.n_writes(); i++) {
        CHECK_EQUAL(MaxSz, mock_writer.frame_size(i));
        CHECK_EQUAL(flags, mock_writer.frame_flags(i));
        CHECK_EQUAL(timestamp + (core::nanoseconds_t)i * core::Second / 2,
                    mock_writer.frame_timestamp(i));

        expect_stereo(mock_writer, MaxSz, 0.3f, 0.3f);
    }

    CHECK_EQUAL(0, mock_writer.num_unread());
}

TEST(channel_mapper_writer, large_frame_downmix) {
    enum { FrameSz = MaxSz * 4 };

    const SampleSpec in_spec(MaxSz, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Stereo);
    const SampleSpec out_spec(MaxSz, ChanLayout_Surround, ChanOrder_Smpte,
                              ChanMask_Surround_Mono);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, buffer_factory, in_spec, out_spec);

    sample_t samples[FrameSz] = {};
    const unsigned flags = Frame::FlagIncomplete;
    const core::nanoseconds_t timestamp = 1000000;

    Frame frame(samples, FrameSz);
    frame.set_flags(flags);
    frame.set_capture_timestamp(timestamp);
    fill_stereo(frame, 0.2f, 0.4f);

    mapper_writer.write(frame);

    CHECK_EQUAL(2, mock_writer.n_writes());

    for (size_t i = 0; i < mock_writer.n_writes(); i++) {
        CHECK_EQUAL(MaxSz, mock_writer.frame_size(i));
        CHECK_EQUAL(flags, mock_writer.frame_flags(i));
        CHECK_EQUAL(timestamp + (core::nanoseconds_t)i * core::Second,
                    mock_writer.frame_timestamp(i));

        expect_mono(mock_writer, MaxSz, 0.3f);
    }

    CHECK_EQUAL(0, mock_writer.num_unread());
}

TEST(channel_mapper_writer, large_frame_nocts) {
    enum { FrameSz = MaxSz * 4 };

    const SampleSpec in_spec(MaxSz, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Stereo);
    const SampleSpec out_spec(MaxSz, ChanLayout_Surround, ChanOrder_Smpte,
                              ChanMask_Surround_Mono);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, buffer_factory, in_spec, out_spec);

    sample_t samples[FrameSz] = {};
    const unsigned flags = Frame::FlagIncomplete;

    Frame frame(samples, FrameSz);
    frame.set_flags(flags);
    frame.set_capture_timestamp(0);
    fill_stereo(frame, 0.2f, 0.4f);

    mapper_writer.write(frame);

    CHECK_EQUAL(2, mock_writer.n_writes());

    for (size_t i = 0; i < mock_writer.n_writes(); i++) {
        CHECK_EQUAL(MaxSz, mock_writer.frame_size(i));
        CHECK_EQUAL(flags, mock_writer.frame_flags(i));
        CHECK_EQUAL(0, mock_writer.frame_timestamp(i));

        expect_mono(mock_writer, MaxSz, 0.3f);
    }

    CHECK_EQUAL(0, mock_writer.num_unread());
}

} // namespace audio
} // namespace roc
