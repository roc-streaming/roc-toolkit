/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/pcm_mapper_writer.h"
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
FrameFactory big_frame_factory(arena, MaxBytes * 10);

FramePtr new_frame(const SampleSpec& sample_spec,
                   size_t n_bytes,
                   unsigned flags,
                   core::nanoseconds_t capt_ts) {
    FramePtr frame = big_frame_factory.allocate_frame(n_bytes);
    CHECK(frame);

    frame->set_raw(sample_spec.is_raw());
    frame->set_flags(flags);
    frame->set_duration(sample_spec.bytes_2_stream_timestamp(n_bytes));
    frame->set_capture_timestamp(capt_ts);

    UNSIGNED_LONGS_EQUAL(n_bytes, frame->num_bytes());

    return frame;
}

void write_frame(IFrameWriter& writer, Frame& frame) {
    LONGS_EQUAL(status::StatusOK, writer.write(frame));
}

template <class T> struct BufferWriter : IFrameWriter {
    T samples[10000];

    int n_calls;
    int n_values;

    const SampleSpec sample_spec;

    BufferWriter(const SampleSpec& sample_spec)
        : n_calls(0)
        , n_values(0)
        , sample_spec(sample_spec) {
        memset(samples, 0, sizeof(samples));
    }

    void reset() {
        n_calls = 0;
        n_values = 0;
    }

    virtual status::StatusCode write(Frame& frame) {
        if (sample_spec.is_raw()) {
            CHECK(frame.is_raw());
            CHECK(frame.raw_samples());

            UNSIGNED_LONGS_EQUAL(frame.duration() * sample_spec.num_channels(),
                                 frame.num_raw_samples());
        } else {
            CHECK(!frame.is_raw());
            CHECK(frame.bytes());
        }

        UNSIGNED_LONGS_EQUAL(frame.duration() * sample_spec.num_channels() * sizeof(T),
                             frame.num_bytes());

        size_t pos = 0;
        while (pos < frame.num_bytes()) {
            CHECK(n_values < (int)ROC_ARRAY_SIZE(samples));
            samples[n_values] = *reinterpret_cast<T*>(frame.bytes() + pos);
            pos += sizeof(T);
            n_values++;
        }

        n_calls++;

        return status::StatusOK;
    }
};

struct MetaWriter : IFrameWriter {
    enum { MaxCalls = 100 };

    packet::stream_timestamp_t duration[MaxCalls];
    unsigned flags[MaxCalls];
    core::nanoseconds_t cts[MaxCalls];

    int n_calls;

    status::StatusCode status;

    const SampleSpec sample_spec;

    MetaWriter(const SampleSpec& sample_spec)
        : n_calls(0)
        , status(status::NoStatus)
        , sample_spec(sample_spec) {
        memset(duration, 0, sizeof(duration));
        memset(flags, 0, sizeof(flags));
        memset(cts, 0, sizeof(cts));
    }

    virtual status::StatusCode write(Frame& frame) {
        if (status != status::NoStatus) {
            return status;
        }

        if (sample_spec.is_raw()) {
            CHECK(frame.is_raw());
            CHECK(frame.raw_samples());

            UNSIGNED_LONGS_EQUAL(frame.duration() * sample_spec.num_channels(),
                                 frame.num_raw_samples());
        } else {
            CHECK(!frame.is_raw());
            CHECK(frame.bytes());
        }

        UNSIGNED_LONGS_EQUAL(sample_spec.stream_timestamp_2_bytes(frame.duration()),
                             frame.num_bytes());

        CHECK(n_calls < MaxCalls);

        duration[n_calls] = frame.duration();
        flags[n_calls] = frame.flags();
        cts[n_calls] = frame.capture_timestamp();

        n_calls++;

        return status::StatusOK;
    }
};

template <class T> struct CountGenerator {
    IFrameWriter& writer;

    T samples[10000];
    T step;

    SampleSpec sample_spec;

    CountGenerator(IFrameWriter& writer, const SampleSpec& sample_spec, T step)
        : writer(writer)
        , step(step)
        , sample_spec(sample_spec) {
    }

    void generate(size_t num) {
        CHECK(num < ROC_ARRAY_SIZE(samples));
        T value = 0;
        for (size_t i = 0; i < num; i++) {
            samples[i] = value;
            value += step;
        }

        const size_t n_bytes = num * sizeof(T);

        FramePtr frame = big_frame_factory.allocate_frame(n_bytes);
        CHECK(frame);
        memcpy(frame->bytes(), samples, n_bytes);
        frame->set_raw(sample_spec.is_raw());
        frame->set_duration(sample_spec.bytes_2_stream_timestamp(n_bytes));

        LONGS_EQUAL(status::StatusOK, writer.write(*frame));
    }
};

} // namespace

TEST_GROUP(pcm_mapper_writer) {};

TEST(pcm_mapper_writer, mono_raw_to_raw) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    BufferWriter<sample_t> buf_writer(out_spec);
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<sample_t> count_generator(mapper_writer, in_spec, 0.001f);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    count_generator.generate(FrameSz);

    LONGS_EQUAL(1, buf_writer.n_calls);
    LONGS_EQUAL(FrameSz, buf_writer.n_values);

    for (size_t i = 0; i < FrameSz; i++) {
        DOUBLES_EQUAL(i * 0.001f, buf_writer.samples[i], Epsilon);
    }
}

TEST(pcm_mapper_writer, mono_s16_to_raw) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    BufferWriter<sample_t> buf_writer(out_spec);
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<int16_t> count_generator(mapper_writer, in_spec, 100);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    count_generator.generate(FrameSz);

    LONGS_EQUAL(1, buf_writer.n_calls);
    LONGS_EQUAL(FrameSz, buf_writer.n_values);

    for (size_t i = 0; i < FrameSz; i++) {
        DOUBLES_EQUAL(i * 100 / 32768., buf_writer.samples[i], Epsilon);
    }
}

TEST(pcm_mapper_writer, mono_raw_to_s16) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    BufferWriter<int16_t> buf_writer(out_spec);
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<sample_t> count_generator(mapper_writer, in_spec, 0.001f);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    count_generator.generate(FrameSz);

    LONGS_EQUAL(1, buf_writer.n_calls);
    LONGS_EQUAL(FrameSz, buf_writer.n_values);

    for (size_t i = 0; i < FrameSz; i++) {
        DOUBLES_EQUAL(i * 0.001, buf_writer.samples[i] / 32768., Epsilon);
    }
}

TEST(pcm_mapper_writer, stereo_s16_to_raw) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Stereo);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    BufferWriter<sample_t> buf_writer(out_spec);
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<int16_t> count_generator(mapper_writer, in_spec, 100);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    count_generator.generate(FrameSz);

    LONGS_EQUAL(1, buf_writer.n_calls);
    LONGS_EQUAL(FrameSz, buf_writer.n_values);

    for (size_t i = 0; i < FrameSz; i++) {
        DOUBLES_EQUAL(i * 100 / 32768., buf_writer.samples[i], Epsilon);
    }
}

TEST(pcm_mapper_writer, stereo_raw_to_s16) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Stereo);
    const SampleSpec out_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    BufferWriter<int16_t> buf_writer(out_spec);
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<sample_t> count_generator(mapper_writer, in_spec, 0.001f);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    count_generator.generate(FrameSz);

    LONGS_EQUAL(1, buf_writer.n_calls);
    LONGS_EQUAL(FrameSz, buf_writer.n_values);

    for (size_t i = 0; i < FrameSz; i++) {
        DOUBLES_EQUAL(i * 0.001, buf_writer.samples[i] / 32768., Epsilon);
    }
}

// Write big frame.
// Frame is split into multiple writes so that output frame fits maximum size.
TEST(pcm_mapper_writer, big_write_s16_to_raw) {
    enum { IterCount = 20, SplitCount = 5, MaxFrameSz = MaxBytes / sizeof(sample_t) };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    BufferWriter<sample_t> buf_writer(out_spec);
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<int16_t> count_generator(mapper_writer, in_spec, 10);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    for (size_t iter = 0; iter < IterCount; iter++) {
        buf_writer.reset();

        count_generator.generate(MaxFrameSz * SplitCount);

        LONGS_EQUAL(SplitCount, buf_writer.n_calls);
        LONGS_EQUAL(MaxFrameSz * SplitCount, buf_writer.n_values);

        for (size_t i = 0; i < MaxFrameSz * SplitCount; i++) {
            DOUBLES_EQUAL(i * 10 / 32768., buf_writer.samples[i], Epsilon);
        }
    }
}

TEST(pcm_mapper_writer, big_write_raw_to_s16) {
    enum { IterCount = 20, SplitCount = 5, MaxFrameSz = MaxBytes / sizeof(int16_t) };

    const SampleSpec in_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    BufferWriter<int16_t> buf_writer(out_spec);
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<sample_t> count_generator(mapper_writer, in_spec, 0.001f);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    for (size_t iter = 0; iter < IterCount; iter++) {
        buf_writer.reset();

        count_generator.generate(MaxFrameSz * SplitCount);

        LONGS_EQUAL(SplitCount, buf_writer.n_calls);
        LONGS_EQUAL(MaxFrameSz * SplitCount, buf_writer.n_values);

        for (size_t i = 0; i < MaxFrameSz * SplitCount; i++) {
            DOUBLES_EQUAL(i * 0.001, buf_writer.samples[i] / 32768., Epsilon);
        }
    }
}

// Check how frame flags are forwarded to inner writer.
TEST(pcm_mapper_writer, forward_flags) {
    enum { MaxFrameSz = MaxBytes / sizeof(int16_t) };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaWriter meta_writer(out_spec);
    PcmMapperWriter mapper_writer(meta_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    {
        const unsigned flags = Frame::HasSignal;
        const core::nanoseconds_t cts = 0;

        FramePtr frame = new_frame(in_spec, MaxFrameSz * 3, flags, cts);
        write_frame(mapper_writer, *frame);
    }

    LONGS_EQUAL(3, meta_writer.n_calls);

    LONGS_EQUAL(Frame::HasSignal, meta_writer.flags[0]);
    LONGS_EQUAL(Frame::HasSignal, meta_writer.flags[1]);
    LONGS_EQUAL(Frame::HasSignal, meta_writer.flags[2]);
}

// Check how frame capture timestamps are forwarded to inner writer.
TEST(pcm_mapper_writer, forward_capture_timestamp) {
    enum { MaxFrameSz = MaxBytes / sizeof(int16_t) };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaWriter meta_writer(out_spec);
    PcmMapperWriter mapper_writer(meta_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    {
        const unsigned flags = 0;
        const core::nanoseconds_t cts = 1000000000;

        FramePtr frame = new_frame(in_spec, MaxFrameSz * 3, flags, cts);
        write_frame(mapper_writer, *frame);
    }

    LONGS_EQUAL(3, meta_writer.n_calls);

    LONGS_EQUAL(1000000000, meta_writer.cts[0]);
    LONGS_EQUAL(1010000000, meta_writer.cts[1]);
    LONGS_EQUAL(1020000000, meta_writer.cts[2]);
}

// Forwarding error from underlying writer.
TEST(pcm_mapper_writer, forward_error) {
    enum { FrameSz = MaxBytes / 10 };

    const SampleSpec in_spec(Rate, PcmSubformat_SInt16, ChanLayout_Surround,
                             ChanOrder_Smpte, ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmSubformat_Raw, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaWriter meta_writer(out_spec);
    PcmMapperWriter mapper_writer(meta_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    meta_writer.status = status::StatusAbort;

    FramePtr frame = new_frame(in_spec, FrameSz, 0, 0);

    LONGS_EQUAL(status::StatusAbort, mapper_writer.write(*frame));
}

} // namespace audio
} // namespace roc
