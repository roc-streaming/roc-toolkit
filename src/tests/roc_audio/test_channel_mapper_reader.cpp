/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_reader.h"

#include "roc_audio/channel_mapper_reader.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"

namespace roc {
namespace audio {

namespace {

const double Epsilon = 0.00001;

enum { MaxSz = 800 };

core::HeapArena arena;
FrameFactory frame_factory(arena, MaxSz * sizeof(sample_t));

void add_mono(test::MockReader& mock_reader,
              size_t size,
              sample_t value,
              unsigned flags) {
    CHECK(size > 0);

    for (size_t n = 0; n < size; n++) {
        mock_reader.add_samples(1, value, flags);
    }
}

void add_stereo(test::MockReader& mock_reader,
                size_t size,
                sample_t left_value,
                sample_t right_value,
                unsigned flags) {
    CHECK(size > 0);
    CHECK(size % 2 == 0);

    for (size_t n = 0; n < size; n += 2) {
        mock_reader.add_samples(1, left_value, flags);
        mock_reader.add_samples(1, right_value, flags);
    }
}

FramePtr expect_frame(status::StatusCode expected_code,
                      IFrameReader& reader,
                      const SampleSpec& sample_spec,
                      size_t requested_samples,
                      size_t expected_samples,
                      FrameReadMode mode = ModeHard) {
    CHECK(requested_samples % sample_spec.num_channels() == 0);
    CHECK(expected_samples % sample_spec.num_channels() == 0);

    FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    const status::StatusCode code =
        reader.read(*frame, requested_samples / sample_spec.num_channels(), mode);

    LONGS_EQUAL(expected_code, code);

    if (expected_code == status::StatusOK || expected_code == status::StatusPart) {
        CHECK(frame->is_raw());

        CHECK(frame->raw_samples());
        CHECK(frame->bytes());

        LONGS_EQUAL(expected_samples / sample_spec.num_channels(), frame->duration());
        LONGS_EQUAL(expected_samples, frame->num_raw_samples());
        LONGS_EQUAL(expected_samples * sizeof(sample_t), frame->num_bytes());
    }

    return frame;
}

void expect_mono(const Frame& frame, size_t n_samples, sample_t value) {
    CHECK(frame.is_raw());

    LONGS_EQUAL(n_samples, frame.num_raw_samples());
    LONGS_EQUAL(n_samples, frame.duration());

    for (size_t n = 0; n < frame.num_raw_samples(); n++) {
        DOUBLES_EQUAL((double)value, (double)frame.raw_samples()[n], Epsilon);
    }
}

void expect_stereo(const Frame& frame,
                   size_t n_samples,
                   sample_t left_value,
                   sample_t right_value) {
    CHECK(frame.is_raw());

    LONGS_EQUAL(n_samples, frame.num_raw_samples());
    LONGS_EQUAL(n_samples / 2, frame.duration());

    for (size_t n = 0; n < frame.num_raw_samples(); n += 2) {
        DOUBLES_EQUAL((double)left_value, (double)frame.raw_samples()[n + 0], Epsilon);
        DOUBLES_EQUAL((double)right_value, (double)frame.raw_samples()[n + 1], Epsilon);
    }
}

} // namespace

TEST_GROUP(channel_mapper_reader) {};

TEST(channel_mapper_reader, small_read_upmix) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    const core::nanoseconds_t start_ts = 1000000;

    test::MockReader mock_reader(frame_factory, in_spec);
    ChannelMapperReader mapper_reader(mock_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    const unsigned flags = Frame::HasSignal;

    mock_reader.enable_timestamps(start_ts);
    add_mono(mock_reader, FrameSz / 2, 0.3f, flags);

    FramePtr frame =
        expect_frame(status::StatusOK, mapper_reader, out_spec, FrameSz, FrameSz);

    LONGS_EQUAL(1, mock_reader.total_reads());
    LONGS_EQUAL(0, mock_reader.num_unread());

    LONGS_EQUAL(flags, frame->flags());
    LONGLONGS_EQUAL(start_ts, frame->capture_timestamp());

    expect_stereo(*frame, FrameSz, 0.3f, 0.3f);
}

TEST(channel_mapper_reader, small_read_downmix) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Stereo);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    const core::nanoseconds_t start_cts = 1000000;

    test::MockReader mock_reader(frame_factory, in_spec);
    ChannelMapperReader mapper_reader(mock_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    const unsigned flags = Frame::HasSignal;

    mock_reader.enable_timestamps(start_cts);
    add_stereo(mock_reader, FrameSz * 2, 0.2f, 0.4f, flags);

    FramePtr frame =
        expect_frame(status::StatusOK, mapper_reader, out_spec, FrameSz, FrameSz);

    LONGS_EQUAL(1, mock_reader.total_reads());
    LONGS_EQUAL(0, mock_reader.num_unread());

    LONGS_EQUAL(flags, frame->flags());
    LONGLONGS_EQUAL(start_cts, frame->capture_timestamp());

    expect_mono(*frame, FrameSz, 0.3f);
}

TEST(channel_mapper_reader, small_read_no_cts) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Stereo);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, in_spec);
    ChannelMapperReader mapper_reader(mock_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    const unsigned flags = Frame::HasSignal;

    add_stereo(mock_reader, FrameSz * 2, 0.2f, 0.4f, flags);

    FramePtr frame =
        expect_frame(status::StatusOK, mapper_reader, out_spec, FrameSz, FrameSz);

    LONGS_EQUAL(1, mock_reader.total_reads());
    LONGS_EQUAL(0, mock_reader.num_unread());

    LONGS_EQUAL(flags, frame->flags());
    LONGS_EQUAL(0, frame->capture_timestamp());

    expect_mono(*frame, FrameSz, 0.3f);
}

// Request big frame when upmixing.
// Duration is capped so that both input and output frames could fit max size.
TEST(channel_mapper_reader, big_read_upmix) {
    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    const core::nanoseconds_t start_cts = 1000000;

    test::MockReader mock_reader(frame_factory, in_spec);
    ChannelMapperReader mapper_reader(mock_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    const unsigned flags1 = Frame::HasSignal;
    const unsigned flags2 = Frame::HasGaps;

    mock_reader.enable_timestamps(start_cts);
    add_mono(mock_reader, MaxSz, 0.3f, flags1);
    add_mono(mock_reader, MaxSz, 0.6f, flags2);

    // MaxSz*2 input samples (1 chan) are mapped to MaxSz*4 output samples (2 chans).
    // Max read size is:
    //   MaxSz/2 input samples = MaxSz output samples.
    // Hence we need 4 partial reads to read all output samples.
    for (int iter = 0; iter < 4; iter++) {
        FramePtr frame =
            expect_frame(status::StatusPart, mapper_reader, out_spec, MaxSz * 2, MaxSz);

        LONGS_EQUAL(iter + 1, mock_reader.total_reads());

        LONGS_EQUAL((iter == 0 || iter == 1) ? flags1 : flags2, frame->flags());
        LONGLONGS_EQUAL(start_cts + out_spec.samples_overall_2_ns(MaxSz) * iter,
                        frame->capture_timestamp());

        const sample_t s = (iter == 0 || iter == 1) ? 0.3f : 0.6f;
        expect_stereo(*frame, MaxSz, s, s);
    }

    LONGS_EQUAL(0, mock_reader.num_unread());
}

// Request big frame when downmixing.
// Duration is capped so that both input and output frames could fit max size.
TEST(channel_mapper_reader, big_read_downmix) {
    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Stereo);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    const core::nanoseconds_t start_cts = 1000000;

    test::MockReader mock_reader(frame_factory, in_spec);
    ChannelMapperReader mapper_reader(mock_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    const unsigned flags1 = Frame::HasSignal;
    const unsigned flags2 = Frame::HasGaps;

    mock_reader.enable_timestamps(start_cts);
    add_stereo(mock_reader, MaxSz * 2, 0.2f, 0.4f, flags1);
    add_stereo(mock_reader, MaxSz * 2, 0.5f, 0.5f, flags2);

    // MaxSz*4 input samples (2 chans) are mapped to MaxSz*2 output samples (1 chan).
    // Max read size is:
    //   MaxSz input samples = MaxSz/2 output samples.
    // Hence we need 4 partial reads to read all output samples.
    for (int iter = 0; iter < 4; iter++) {
        FramePtr frame = expect_frame(status::StatusPart, mapper_reader, out_spec,
                                      MaxSz * 2, MaxSz / 2);

        LONGS_EQUAL(iter + 1, mock_reader.total_reads());

        LONGS_EQUAL((iter == 0 || iter == 1) ? flags1 : flags2, frame->flags());
        LONGLONGS_EQUAL(start_cts + out_spec.samples_overall_2_ns(MaxSz / 2) * iter,
                        frame->capture_timestamp());

        const sample_t s = (iter == 0 || iter == 1) ? 0.3f : 0.5f;
        expect_mono(*frame, MaxSz / 2, s);
    }

    LONGS_EQUAL(0, mock_reader.num_unread());
}

// Same as above, but input frames don't have CTS
// (because we don't call enable_timestamps).
TEST(channel_mapper_reader, big_read_no_cts) {
    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Stereo);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    test::MockReader mock_reader(frame_factory, in_spec);
    ChannelMapperReader mapper_reader(mock_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    const unsigned flags1 = Frame::HasSignal;
    const unsigned flags2 = Frame::HasGaps;

    add_stereo(mock_reader, MaxSz * 2, 0.2f, 0.4f, flags1);
    add_stereo(mock_reader, MaxSz * 2, 0.5f, 0.5f, flags2);

    for (int iter = 0; iter < 4; iter++) {
        FramePtr frame = expect_frame(status::StatusPart, mapper_reader, out_spec,
                                      MaxSz * 2, MaxSz / 2);

        LONGS_EQUAL(iter + 1, mock_reader.total_reads());

        LONGS_EQUAL((iter == 0 || iter == 1) ? flags1 : flags2, frame->flags());
        LONGLONGS_EQUAL(0, frame->capture_timestamp());

        const sample_t s = (iter == 0 || iter == 1) ? 0.3f : 0.5f;
        expect_mono(*frame, MaxSz / 2, s);
    }

    LONGS_EQUAL(0, mock_reader.num_unread());
}

// Forwarding mode to underlying reader.
TEST(channel_mapper_reader, forward_mode) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    test::MockReader mock_reader(frame_factory, in_spec);
    ChannelMapperReader mapper_reader(mock_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    mock_reader.add_zero_samples();

    const FrameReadMode mode_list[] = {
        ModeHard,
        ModeSoft,
    };

    for (size_t md_n = 0; md_n < ROC_ARRAY_SIZE(mode_list); md_n++) {
        FramePtr frame = expect_frame(status::StatusOK, mapper_reader, out_spec, FrameSz,
                                      FrameSz, mode_list[md_n]);

        LONGS_EQUAL(mode_list[md_n], mock_reader.last_mode());
    }
}

// Forwarding error from underlying reader.
TEST(channel_mapper_reader, forward_error) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    test::MockReader mock_reader(frame_factory, in_spec);
    ChannelMapperReader mapper_reader(mock_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    const status::StatusCode status_list[] = {
        status::StatusDrain,
        status::StatusAbort,
    };

    for (size_t st_n = 0; st_n < ROC_ARRAY_SIZE(status_list); st_n++) {
        mock_reader.set_status(status_list[st_n]);

        FramePtr frame =
            expect_frame(status_list[st_n], mapper_reader, out_spec, FrameSz, 0);
    }
}

// Forwarding partial read from underlying reader.
TEST(channel_mapper_reader, forward_partial) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    test::MockReader mock_reader(frame_factory, in_spec);
    ChannelMapperReader mapper_reader(mock_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    add_mono(mock_reader, FrameSz / 4, 0.1f, 0);

    FramePtr frame =
        expect_frame(status::StatusPart, mapper_reader, out_spec, FrameSz, FrameSz / 2);

    LONGS_EQUAL(status::StatusPart, mock_reader.last_status());

    LONGS_EQUAL(1, mock_reader.total_reads());
    LONGS_EQUAL(0, mock_reader.num_unread());
}

// Attach to frame pre-allocated buffers of different sizes before reading.
TEST(channel_mapper_reader, preallocated_buffer) {
    enum { FrameSz = MaxSz / 2 };

    const SampleSpec in_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(MaxSz, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    const size_t buffer_list[] = {
        FrameSz * 50, // big size (reader should use it)
        FrameSz,      // exact size (reader should use it)
        FrameSz - 1,  // small size (reader should replace buffer)
        0,            // no buffer (reader should allocate buffer)
    };

    for (size_t bn = 0; bn < ROC_ARRAY_SIZE(buffer_list); bn++) {
        const size_t orig_buf_sz = buffer_list[bn];

        test::MockReader mock_reader(frame_factory, in_spec);
        mock_reader.add_zero_samples();

        ChannelMapperReader mapper_reader(mock_reader, frame_factory, in_spec, out_spec);
        LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

        FrameFactory mock_factory(arena, orig_buf_sz * sizeof(sample_t));
        FramePtr frame = orig_buf_sz > 0 ? mock_factory.allocate_frame(0)
                                         : mock_factory.allocate_frame_no_buffer();

        core::Slice<uint8_t> orig_buf = frame->buffer();

        LONGS_EQUAL(
            status::StatusOK,
            mapper_reader.read(*frame, FrameSz / out_spec.num_channels(), ModeHard));

        CHECK(frame->buffer());

        if (orig_buf_sz >= FrameSz) {
            CHECK(frame->buffer() == orig_buf);
        } else {
            CHECK(frame->buffer() != orig_buf);
        }

        LONGS_EQUAL(FrameSz / out_spec.num_channels(), frame->duration());
        LONGS_EQUAL(FrameSz, frame->num_raw_samples());
        LONGS_EQUAL(FrameSz * sizeof(sample_t), frame->num_bytes());
    }
}

} // namespace audio
} // namespace roc
