/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/pcm_mapper_reader.h"
#include "roc_audio/pcm_subformat.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

namespace {

const double Epsilon = 0.0001;

enum { Rate = 10000, MaxBytes = 400 };

core::HeapArena arena;
FrameFactory frame_factory(arena, MaxBytes);

FramePtr expect_raw_frame(status::StatusCode expected_code,
                          IFrameReader& reader,
                          const SampleSpec& sample_spec,
                          size_t requested_samples,
                          size_t expected_samples,
                          FrameReadMode mode = ModeHard) {
    CHECK(requested_samples % sample_spec.num_channels() == 0);

    FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    LONGS_EQUAL(
        expected_code,
        reader.read(*frame, requested_samples / sample_spec.num_channels(), mode));

    if (expected_code == status::StatusOK || expected_code == status::StatusPart) {
        CHECK(frame->is_raw());
        CHECK(frame->raw_samples());
        CHECK(frame->bytes());

        UNSIGNED_LONGS_EQUAL(expected_samples / sample_spec.num_channels(),
                             frame->duration());
        UNSIGNED_LONGS_EQUAL(expected_samples, frame->num_raw_samples());
        UNSIGNED_LONGS_EQUAL(expected_samples * sizeof(sample_t), frame->num_bytes());
    }

    return frame;
}

FramePtr expect_byte_frame(status::StatusCode expected_code,
                           IFrameReader& reader,
                           const SampleSpec& sample_spec,
                           size_t requested_samples,
                           size_t expected_samples,
                           FrameReadMode mode = ModeHard) {
    CHECK(requested_samples % sample_spec.num_channels() == 0);

    FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    LONGS_EQUAL(
        expected_code,
        reader.read(*frame, requested_samples / sample_spec.num_channels(), mode));

    if (expected_samples != 0) {
        CHECK(!frame->is_raw());
        CHECK(frame->bytes());

        UNSIGNED_LONGS_EQUAL(expected_samples / sample_spec.num_channels(),
                             frame->duration());
        UNSIGNED_LONGS_EQUAL(
            sample_spec.stream_timestamp_2_bytes(packet::stream_timestamp_t(
                expected_samples / sample_spec.num_channels())),
            frame->num_bytes());
    }

    return frame;
}

template <class T> struct CountReader : IFrameReader {
    T value;
    T step;

    int n_calls;
    int n_values;

    int limit_values;

    status::StatusCode last_status;

    const SampleSpec sample_spec;

    CountReader(const SampleSpec& sample_spec, T step)
        : value(0)
        , step(step)
        , n_calls(0)
        , n_values(0)
        , limit_values(0)
        , last_status(status::NoStatus)
        , sample_spec(sample_spec) {
    }

    void reset() {
        value = 0;
        n_calls = 0;
        n_values = 0;
    }

    virtual status::StatusCode read(Frame& frame,
                                    packet::stream_timestamp_t requested_duration,
                                    FrameReadMode mode) {
        n_calls++;

        packet::stream_timestamp_t duration = requested_duration;

        if (limit_values != 0) {
            duration =
                std::min(duration,
                         packet::stream_timestamp_t((limit_values - n_values)
                                                    / (int)sample_spec.num_channels()));
        }

        if (duration == 0) {
            return (last_status = status::StatusFinish);
        }

        CHECK(frame_factory.reallocate_frame(
            frame, sample_spec.stream_timestamp_2_bytes(duration)));

        frame.set_raw(sample_spec.is_raw());
        frame.set_duration(duration);

        size_t pos = 0;
        while (pos < frame.num_bytes()) {
            *reinterpret_cast<T*>(frame.bytes() + pos) = value;
            value += step;
            pos += sizeof(T);
            n_values++;
        }

        return (last_status = (duration == requested_duration ? status::StatusOK
                                                              : status::StatusPart));
    }
};

struct MetaReader : IFrameReader {
    unsigned flags[10];
    core::nanoseconds_t cts[10];
    unsigned pos;

    int n_calls;
    FrameReadMode last_mode;

    status::StatusCode status;

    const SampleSpec sample_spec;

    MetaReader(const SampleSpec& sample_spec)
        : pos(0)
        , n_calls(0)
        , last_mode((FrameReadMode)-1)
        , status(status::NoStatus)
        , sample_spec(sample_spec) {
        memset(flags, 0, sizeof(flags));
        memset(cts, 0, sizeof(cts));
    }

    virtual status::StatusCode
    read(Frame& frame, packet::stream_timestamp_t duration, FrameReadMode mode) {
        if (status != status::NoStatus) {
            return status;
        }

        CHECK(pos < ROC_ARRAY_SIZE(flags));
        CHECK(pos < ROC_ARRAY_SIZE(cts));

        CHECK(frame_factory.reallocate_frame(
            frame, sample_spec.stream_timestamp_2_bytes(duration)));

        frame.set_raw(sample_spec.is_raw());
        frame.set_duration(duration);

        frame.set_flags(flags[pos]);
        frame.set_capture_timestamp(cts[pos]);

        pos++;
        n_calls++;
        last_mode = mode;

        return status::StatusOK;
    }
};

} // namespace

TEST_GROUP(pcm_mapper_reader) {};

TEST(pcm_mapper_reader, mono_raw_to_raw) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<sample_t> count_reader(in_spec, 0.001f);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    FramePtr frame =
        expect_raw_frame(status::StatusOK, mapper_reader, out_spec, FrameSz, FrameSz);

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(FrameSz, count_reader.n_values);

    const sample_t* samples = frame->raw_samples();

    for (size_t i = 0; i < FrameSz; i++) {
        DOUBLES_EQUAL(i * 0.001, samples[i], Epsilon);
    }
}

TEST(pcm_mapper_reader, mono_s16_to_raw) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<int16_t> count_reader(in_spec, 100);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    FramePtr frame =
        expect_raw_frame(status::StatusOK, mapper_reader, out_spec, FrameSz, FrameSz);

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(FrameSz, count_reader.n_values);

    const sample_t* samples = frame->raw_samples();

    for (size_t i = 0; i < FrameSz; i++) {
        DOUBLES_EQUAL(i * 100 / 32768., samples[i], Epsilon);
    }
}

TEST(pcm_mapper_reader, mono_raw_to_s16) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<sample_t> count_reader(in_spec, 0.001f);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    FramePtr frame =
        expect_byte_frame(status::StatusOK, mapper_reader, out_spec, FrameSz, FrameSz);

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(FrameSz, count_reader.n_values);

    const int16_t* samples = (const int16_t*)frame->bytes();

    for (size_t i = 0; i < FrameSz; i++) {
        DOUBLES_EQUAL(i * 0.001, samples[i] / 32768., Epsilon);
    }
}

TEST(pcm_mapper_reader, stereo_s16_to_raw) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Stereo);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    CountReader<int16_t> count_reader(in_spec, 100);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    FramePtr frame =
        expect_raw_frame(status::StatusOK, mapper_reader, out_spec, FrameSz, FrameSz);

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(FrameSz, count_reader.n_values);

    const sample_t* samples = frame->raw_samples();

    for (size_t i = 0; i < FrameSz; i++) {
        DOUBLES_EQUAL(i * 100 / 32768., samples[i], Epsilon);
    }
}

TEST(pcm_mapper_reader, stereo_raw_to_s16) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Stereo);
    const SampleSpec out_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    CountReader<sample_t> count_reader(in_spec, 0.001f);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    FramePtr frame =
        expect_byte_frame(status::StatusOK, mapper_reader, out_spec, FrameSz, FrameSz);

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(FrameSz, count_reader.n_values);

    const int16_t* samples = (const int16_t*)frame->bytes();

    for (size_t i = 0; i < FrameSz; i++) {
        DOUBLES_EQUAL(i * 0.001, samples[i] / 32768., Epsilon);
    }
}

// Request big frame.
// Duration is capped so that both input and output frames could fit max size.
TEST(pcm_mapper_reader, big_read_s16_to_raw) {
    enum {
        IterCount = 5,
        MaxFrameSz = MaxBytes / ROC_MAX(sizeof(int16_t), sizeof(sample_t))
    };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<int16_t> count_reader(in_spec, 10);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    size_t pos = 0;

    for (int iter = 0; iter < IterCount; iter++) {
        FramePtr frame = expect_raw_frame(status::StatusPart, mapper_reader, out_spec,
                                          MaxFrameSz * 3, MaxFrameSz);

        LONGS_EQUAL(iter + 1, count_reader.n_calls);
        LONGS_EQUAL(MaxFrameSz * (iter + 1), count_reader.n_values);

        const sample_t* samples = frame->raw_samples();

        for (size_t i = 0; i < MaxFrameSz; i++) {
            DOUBLES_EQUAL(pos * 10 / 32768., samples[i], Epsilon);
            pos++;
        }
    }
}

// Similar to above.
TEST(pcm_mapper_reader, big_read_raw_to_s16) {
    enum {
        IterCount = 5,
        MaxFrameSz = MaxBytes / ROC_MAX(sizeof(int16_t), sizeof(sample_t))
    };

    const SampleSpec in_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<sample_t> count_reader(in_spec, 0.001f);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    size_t pos = 0;

    for (int iter = 0; iter < IterCount; iter++) {
        FramePtr frame = expect_byte_frame(status::StatusPart, mapper_reader, out_spec,
                                           MaxFrameSz * 3, MaxFrameSz);

        LONGS_EQUAL(iter + 1, count_reader.n_calls);
        LONGS_EQUAL(MaxFrameSz * (iter + 1), count_reader.n_values);

        const int16_t* samples = (const int16_t*)frame->bytes();

        for (size_t i = 0; i < MaxFrameSz; i++) {
            DOUBLES_EQUAL(pos * 0.001, samples[i] / 32768., Epsilon);
            pos++;
        }
    }
}

// Check how frame flags are forwarded from inner reader.
TEST(pcm_mapper_reader, forward_flags) {
    enum { MaxFrameSz = MaxBytes / sizeof(sample_t) };

    const SampleSpec in_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaReader meta_reader(in_spec);
    PcmMapperReader mapper_reader(meta_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    meta_reader.flags[0] = 0;
    meta_reader.flags[1] = Frame::HasSignal;
    meta_reader.flags[2] = Frame::HasGaps;

    for (int iter = 0; iter < 3; iter++) {
        FramePtr frame = expect_raw_frame(status::StatusPart, mapper_reader, out_spec,
                                          MaxFrameSz * 3, MaxFrameSz);

        LONGS_EQUAL(iter + 1, meta_reader.n_calls);

        UNSIGNED_LONGS_EQUAL(meta_reader.flags[iter], frame->flags());
    }
}

// Check how frame capture timestamps are forwarded from inner reader.
TEST(pcm_mapper_reader, forward_capture_timestamp) {
    enum { MaxFrameSz = MaxBytes / sizeof(sample_t) };

    const SampleSpec in_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaReader meta_reader(in_spec);
    PcmMapperReader mapper_reader(meta_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    meta_reader.cts[0] = 10000000000;
    meta_reader.cts[1] = 20000000000;
    meta_reader.cts[2] = 30000000000;

    for (int iter = 0; iter < 3; iter++) {
        FramePtr frame = expect_raw_frame(status::StatusPart, mapper_reader, out_spec,
                                          MaxFrameSz * 3, MaxFrameSz);

        LONGS_EQUAL(iter + 1, meta_reader.n_calls);

        LONGLONGS_EQUAL(meta_reader.cts[iter], frame->capture_timestamp());
    }
}

// Forwarding mode to underlying reader.
TEST(pcm_mapper_reader, forward_mode) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaReader meta_reader(in_spec);
    PcmMapperReader mapper_reader(meta_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    const FrameReadMode mode_list[] = {
        ModeHard,
        ModeSoft,
    };

    for (size_t md_n = 0; md_n < ROC_ARRAY_SIZE(mode_list); md_n++) {
        FramePtr frame = expect_raw_frame(status::StatusOK, mapper_reader, out_spec,
                                          FrameSz, FrameSz, mode_list[md_n]);

        LONGS_EQUAL(mode_list[md_n], meta_reader.last_mode);
    }
}

// Forwarding error from underlying reader.
TEST(pcm_mapper_reader, forward_error) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaReader meta_reader(in_spec);
    PcmMapperReader mapper_reader(meta_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    const status::StatusCode status_list[] = {
        status::StatusDrain,
        status::StatusAbort,
    };

    for (size_t st_n = 0; st_n < ROC_ARRAY_SIZE(status_list); st_n++) {
        meta_reader.status = status_list[st_n];

        FramePtr frame =
            expect_raw_frame(status_list[st_n], mapper_reader, out_spec, FrameSz, 0);
    }
}

// Forwarding partial read from underlying reader.
TEST(pcm_mapper_reader, forward_partial) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<int16_t> count_reader(in_spec, 100);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    count_reader.limit_values = FrameSz / 2;

    FramePtr frame = expect_raw_frame(status::StatusPart, mapper_reader, out_spec,
                                      FrameSz, FrameSz / 2);

    LONGS_EQUAL(status::StatusPart, count_reader.last_status);

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(FrameSz / 2, count_reader.n_values);
}

// Attach to frame pre-allocated buffers of different sizes before reading.
TEST(pcm_mapper_reader, preallocated_buffer) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    const size_t buffer_list[] = {
        FrameSz * 50, // big size (reader should use it)
        FrameSz,      // exact size (reader should use it)
        FrameSz - 1,  // small size (reader should replace buffer)
        0,            // no buffer (reader should allocate buffer)
    };

    for (size_t bn = 0; bn < ROC_ARRAY_SIZE(buffer_list); bn++) {
        const size_t orig_buf_sz = buffer_list[bn];

        CountReader<int16_t> count_reader(in_spec, 100);
        PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
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
