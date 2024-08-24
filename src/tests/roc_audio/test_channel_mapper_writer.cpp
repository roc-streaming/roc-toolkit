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
#include "roc_core/heap_arena.h"

namespace roc {
namespace audio {

namespace {

const double Epsilon = 0.00001;

enum { MaxSz = 500 };

core::HeapArena arena;
FrameFactory frame_factory(arena, MaxSz * sizeof(sample_t));
FrameFactory big_frame_factory(arena, MaxSz * 10 * sizeof(sample_t));

FramePtr new_frame(const SampleSpec& sample_spec,
                   size_t n_samples,
                   unsigned flags,
                   core::nanoseconds_t capt_ts) {
    CHECK(n_samples % sample_spec.num_channels() == 0);

    FramePtr frame = big_frame_factory.allocate_frame(n_samples * sizeof(sample_t));
    CHECK(frame);

    frame->set_raw(true);
    frame->set_flags(flags);
    frame->set_duration(
        packet::stream_timestamp_t(n_samples / sample_spec.num_channels()));
    frame->set_capture_timestamp(capt_ts);

    UNSIGNED_LONGS_EQUAL(n_samples, frame->num_raw_samples());

    return frame;
}

void write_frame(IFrameWriter& writer, Frame& frame) {
    LONGS_EQUAL(status::StatusOK, writer.write(frame));
}

void fill_mono(Frame& frame, sample_t value) {
    CHECK(frame.num_raw_samples() > 0);

    for (size_t n = 0; n < frame.num_raw_samples(); n++) {
        frame.raw_samples()[n] = value;
    }
}

void fill_stereo(Frame& frame, sample_t left_value, sample_t right_value) {
    CHECK(frame.num_raw_samples() > 0);
    CHECK(frame.num_raw_samples() % 2 == 0);

    for (size_t n = 0; n < frame.num_raw_samples(); n += 2) {
        frame.raw_samples()[n + 0] = left_value;
        frame.raw_samples()[n + 1] = right_value;
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

TEST(channel_mapper_writer, small_write_upmix) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    const unsigned flags = Frame::HasSignal;
    const core::nanoseconds_t capt_ts = 1000000;

    FramePtr frame = new_frame(in_spec, FrameSz, flags, capt_ts);
    fill_mono(*frame, 0.3f);
    write_frame(mapper_writer, *frame);

    LONGS_EQUAL(1, mock_writer.n_writes());

    LONGS_EQUAL(FrameSz * 2, mock_writer.frame_size(0));
    LONGS_EQUAL(flags, mock_writer.frame_flags(0));
    LONGLONGS_EQUAL(capt_ts, mock_writer.frame_timestamp(0));

    expect_stereo(mock_writer, FrameSz * 2, 0.3f, 0.3f);

    LONGS_EQUAL(0, mock_writer.num_unread());
}

TEST(channel_mapper_writer, small_write_downmix) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Stereo);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    const unsigned flags = Frame::HasSignal;
    const core::nanoseconds_t capt_ts = 1000000;

    FramePtr frame = new_frame(in_spec, FrameSz, flags, capt_ts);
    fill_stereo(*frame, 0.2f, 0.4f);
    write_frame(mapper_writer, *frame);

    LONGS_EQUAL(1, mock_writer.n_writes());

    LONGS_EQUAL(FrameSz / 2, mock_writer.frame_size(0));
    LONGS_EQUAL(flags, mock_writer.frame_flags(0));
    LONGLONGS_EQUAL(capt_ts, mock_writer.frame_timestamp(0));

    expect_mono(mock_writer, FrameSz / 2, 0.3f);

    LONGS_EQUAL(0, mock_writer.num_unread());
}

TEST(channel_mapper_writer, small_write_no_cts) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Stereo);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    const unsigned flags = Frame::HasSignal;

    FramePtr frame = new_frame(in_spec, FrameSz, flags, 0);
    fill_stereo(*frame, 0.2f, 0.4f);
    write_frame(mapper_writer, *frame);

    LONGS_EQUAL(1, mock_writer.n_writes());

    LONGS_EQUAL(FrameSz / 2, mock_writer.frame_size(0));
    LONGS_EQUAL(flags, mock_writer.frame_flags(0));
    LONGLONGS_EQUAL(0, mock_writer.frame_timestamp(0));

    expect_mono(mock_writer, FrameSz / 2, 0.3f);

    LONGS_EQUAL(0, mock_writer.num_unread());
}

// Write big frame when upmixing.
// It should be split into multiple writes to fit maximum size.
TEST(channel_mapper_writer, big_write_upmix) {
    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    const unsigned flags = Frame::HasSignal;
    const core::nanoseconds_t capt_ts = 1000000;

    // MaxSz*3 input samples (1 chan) are mapped to MaxSz*6 output samples (2 chans).
    // Max write size is MaxSz, so we expect 6 writes.
    FramePtr frame = new_frame(in_spec, MaxSz * 3, flags, capt_ts);
    fill_mono(*frame, 0.3f);
    write_frame(mapper_writer, *frame);

    LONGS_EQUAL(6, mock_writer.n_writes());

    for (size_t i = 0; i < mock_writer.n_writes(); i++) {
        LONGS_EQUAL(MaxSz, mock_writer.frame_size(i));
        LONGS_EQUAL(flags, mock_writer.frame_flags(i));
        LONGLONGS_EQUAL(capt_ts + core::nanoseconds_t(i) * core::Second / 2,
                        mock_writer.frame_timestamp(i));

        expect_stereo(mock_writer, MaxSz, 0.3f, 0.3f);
    }

    LONGS_EQUAL(0, mock_writer.num_unread());
}

// Write big frame when downmixing.
// It should be split into multiple writes to fit maximum size.
TEST(channel_mapper_writer, big_write_downmix) {
    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Stereo);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    const unsigned flags = Frame::HasSignal;
    const core::nanoseconds_t capt_ts = 1000000;

    // MaxSz*4 input samples (2 chans) are mapped to MaxSz*2 output samples (1 chan).
    // Max write size is MaxSz, so we expect 2 writes.
    FramePtr frame = new_frame(in_spec, MaxSz * 4, flags, capt_ts);
    fill_stereo(*frame, 0.2f, 0.4f);
    write_frame(mapper_writer, *frame);

    LONGS_EQUAL(2, mock_writer.n_writes());

    for (size_t i = 0; i < mock_writer.n_writes(); i++) {
        LONGS_EQUAL(MaxSz, mock_writer.frame_size(i));
        LONGS_EQUAL(flags, mock_writer.frame_flags(i));
        LONGLONGS_EQUAL(capt_ts + (core::nanoseconds_t)i * core::Second,
                        mock_writer.frame_timestamp(i));

        expect_mono(mock_writer, MaxSz, 0.3f);
    }

    LONGS_EQUAL(0, mock_writer.num_unread());
}

// Same as above, but input frames don't have CTS.
TEST(channel_mapper_writer, big_write_no_cts) {
    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Stereo);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    const unsigned flags = Frame::HasSignal;

    FramePtr frame = new_frame(in_spec, MaxSz * 4, flags, 0);
    fill_stereo(*frame, 0.2f, 0.4f);
    write_frame(mapper_writer, *frame);

    LONGS_EQUAL(2, mock_writer.n_writes());

    for (size_t i = 0; i < mock_writer.n_writes(); i++) {
        LONGS_EQUAL(MaxSz, mock_writer.frame_size(i));
        LONGS_EQUAL(flags, mock_writer.frame_flags(i));
        LONGLONGS_EQUAL(0, mock_writer.frame_timestamp(i));

        expect_mono(mock_writer, MaxSz, 0.3f);
    }

    LONGS_EQUAL(0, mock_writer.num_unread());
}

// Forwarding error from underlying writer.
TEST(channel_mapper_writer, forward_error) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    test::MockWriter mock_writer;
    ChannelMapperWriter mapper_writer(mock_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    mock_writer.set_status(status::StatusAbort);

    FramePtr frame = new_frame(in_spec, FrameSz, 0, 0);
    CHECK(frame);

    LONGS_EQUAL(status::StatusAbort, mapper_writer.write(*frame));
}

} // namespace audio
} // namespace roc
