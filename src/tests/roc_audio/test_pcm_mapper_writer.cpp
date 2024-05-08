/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/pcm_format.h"
#include "roc_audio/pcm_mapper_writer.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

namespace {

const double Epsilon = 0.0001;

enum { Rate = 10000, MaxBytes = 400, SmallFrameSz = 20 };

core::HeapArena arena;
FrameFactory frame_factory(arena, MaxBytes);

template <class T> struct BufferWriter : IFrameWriter {
    T samples[10000];

    int n_calls;
    int n_values;

    BufferWriter()
        : n_calls(0)
        , n_values(0) {
        memset(samples, 0, sizeof(samples));
    }

    void reset() {
        n_calls = 0;
        n_values = 0;
    }

    virtual void write(Frame& frame) {
        size_t pos = 0;
        while (pos < frame.num_bytes()) {
            CHECK(n_values < (int)ROC_ARRAY_SIZE(samples));
            samples[n_values] = *reinterpret_cast<T*>(frame.bytes() + pos);
            pos += sizeof(T);
            n_values++;
        }
        n_calls++;
    }
};

struct MetaWriter : IFrameWriter {
    enum { MaxCalls = 10 };

    packet::stream_timestamp_t duration[MaxCalls];
    unsigned flags[MaxCalls];
    core::nanoseconds_t cts[MaxCalls];

    int n_calls;

    MetaWriter()
        : n_calls(0) {
        memset(duration, 0, sizeof(duration));
        memset(flags, 0, sizeof(flags));
        memset(cts, 0, sizeof(cts));
    }

    virtual void write(Frame& frame) {
        CHECK(n_calls < MaxCalls);
        duration[n_calls] = frame.duration();
        flags[n_calls] = frame.flags();
        cts[n_calls] = frame.capture_timestamp();
        n_calls++;
    }
};

template <class T> struct CountGenerator {
    IFrameWriter& writer;

    T samples[10000];
    T step;

    CountGenerator(IFrameWriter& writer, T step)
        : writer(writer)
        , step(step) {
    }

    void generate(size_t num) {
        CHECK(num < ROC_ARRAY_SIZE(samples));
        T value = 0;
        for (size_t i = 0; i < num; i++) {
            samples[i] = value;
            value += step;
        }

        Frame frame((uint8_t*)samples, num * sizeof(T));
        writer.write(frame);
    }
};

} // namespace

TEST_GROUP(pcm_mapper_writer) {};

TEST(pcm_mapper_writer, raw_to_raw) {
    const SampleSpec in_spec(Rate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    BufferWriter<sample_t> buf_writer;
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<sample_t> count_generator(mapper_writer, 0.001f);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    count_generator.generate(SmallFrameSz);

    LONGS_EQUAL(1, buf_writer.n_calls);
    LONGS_EQUAL(SmallFrameSz, buf_writer.n_values);

    for (size_t i = 0; i < SmallFrameSz; i++) {
        DOUBLES_EQUAL(i * 0.001f, buf_writer.samples[i], Epsilon);
    }
}

TEST(pcm_mapper_writer, s16_to_raw) {
    const SampleSpec in_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    BufferWriter<sample_t> buf_writer;
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<int16_t> count_generator(mapper_writer, 100);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    count_generator.generate(SmallFrameSz);

    LONGS_EQUAL(1, buf_writer.n_calls);
    LONGS_EQUAL(SmallFrameSz, buf_writer.n_values);

    for (size_t i = 0; i < SmallFrameSz; i++) {
        DOUBLES_EQUAL(i * 100 / 32768., buf_writer.samples[i], Epsilon);
    }
}

TEST(pcm_mapper_writer, raw_to_s16) {
    const SampleSpec in_spec(Rate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    BufferWriter<int16_t> buf_writer;
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<sample_t> count_generator(mapper_writer, 0.001f);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    count_generator.generate(SmallFrameSz);

    LONGS_EQUAL(1, buf_writer.n_calls);
    LONGS_EQUAL(SmallFrameSz, buf_writer.n_values);

    for (size_t i = 0; i < SmallFrameSz; i++) {
        DOUBLES_EQUAL(i * 0.001, buf_writer.samples[i] / 32768., Epsilon);
    }
}

TEST(pcm_mapper_writer, s16_to_s32) {
    const SampleSpec in_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmFormat_SInt32, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    BufferWriter<int32_t> buf_writer;
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<int16_t> count_generator(mapper_writer, 100);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    count_generator.generate(SmallFrameSz);

    LONGS_EQUAL(1, buf_writer.n_calls);
    LONGS_EQUAL(SmallFrameSz, buf_writer.n_values);

    for (size_t i = 0; i < SmallFrameSz; i++) {
        DOUBLES_EQUAL(i * 100 / 32768., buf_writer.samples[i] / 2147483648., Epsilon);
    }
}

TEST(pcm_mapper_writer, s32_to_s16) {
    const SampleSpec in_spec(Rate, PcmFormat_SInt32, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    BufferWriter<int16_t> buf_writer;
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<int32_t> count_generator(mapper_writer, 1000);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    count_generator.generate(SmallFrameSz);

    LONGS_EQUAL(1, buf_writer.n_calls);
    LONGS_EQUAL(SmallFrameSz, buf_writer.n_values);

    for (size_t i = 0; i < SmallFrameSz; i++) {
        DOUBLES_EQUAL(i * 1000 / 2147483648., buf_writer.samples[i] / 32768., Epsilon);
    }
}

// Pass large input frame, so that internally it's split into multiple
// smaller output frames.
TEST(pcm_mapper_writer, split_frame) {
    enum { SplitCount = 10, LargeFrameSz = MaxBytes / sizeof(sample_t) * SplitCount };

    const SampleSpec in_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    BufferWriter<sample_t> buf_writer;
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<int16_t> count_generator(mapper_writer, 10);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    count_generator.generate(LargeFrameSz);

    LONGS_EQUAL(SplitCount, buf_writer.n_calls);
    LONGS_EQUAL(LargeFrameSz, buf_writer.n_values);

    for (size_t i = 0; i < LargeFrameSz; i++) {
        DOUBLES_EQUAL(i * 10 / 32768., buf_writer.samples[i], Epsilon);
    }
}

// Same, but repeatedly.
TEST(pcm_mapper_writer, split_frame_loop) {
    enum {
        IterCount = 20,
        SplitCount = 10,
        LargeFrameSz = MaxBytes / sizeof(sample_t) * SplitCount
    };

    const SampleSpec in_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    BufferWriter<sample_t> buf_writer;
    PcmMapperWriter mapper_writer(buf_writer, frame_factory, in_spec, out_spec);
    CountGenerator<int16_t> count_generator(mapper_writer, 10);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    for (size_t iter = 0; iter < IterCount; iter++) {
        buf_writer.reset();
        count_generator.generate(LargeFrameSz);

        LONGS_EQUAL(SplitCount, buf_writer.n_calls);
        LONGS_EQUAL(LargeFrameSz, buf_writer.n_values);

        for (size_t i = 0; i < LargeFrameSz; i++) {
            DOUBLES_EQUAL(i * 10 / 32768., buf_writer.samples[i], Epsilon);
        }
    }
}

TEST(pcm_mapper_writer, duration_mono) {
    enum { MaxSamples = MaxBytes / sizeof(sample_t) };

    const SampleSpec in_spec(Rate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaWriter meta_writer;
    PcmMapperWriter mapper_writer(meta_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    {
        sample_t samples[MaxSamples * 3] = {};
        Frame frame(samples, MaxSamples * 3);
        mapper_writer.write(frame);
    }

    LONGS_EQUAL(3, meta_writer.n_calls);

    LONGS_EQUAL(MaxSamples, meta_writer.duration[0]);
    LONGS_EQUAL(MaxSamples, meta_writer.duration[1]);
    LONGS_EQUAL(MaxSamples, meta_writer.duration[2]);
}

TEST(pcm_mapper_writer, duration_stereo) {
    enum { MaxSamples = MaxBytes / sizeof(sample_t) };

    const SampleSpec in_spec(Rate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Stereo);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    MetaWriter meta_writer;
    PcmMapperWriter mapper_writer(meta_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    {
        sample_t samples[MaxSamples * 3] = {};
        Frame frame(samples, MaxSamples * 3);
        mapper_writer.write(frame);
    }

    LONGS_EQUAL(3, meta_writer.n_calls);

    LONGS_EQUAL(MaxSamples / 2, meta_writer.duration[0]);
    LONGS_EQUAL(MaxSamples / 2, meta_writer.duration[1]);
    LONGS_EQUAL(MaxSamples / 2, meta_writer.duration[2]);
}

TEST(pcm_mapper_writer, flags_to_raw) {
    enum { MaxSamples = MaxBytes / sizeof(sample_t) };

    const SampleSpec in_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaWriter meta_writer;
    PcmMapperWriter mapper_writer(meta_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    {
        int16_t samples[MaxSamples * 3] = {};
        Frame frame((uint8_t*)samples, sizeof(samples));
        frame.set_flags(Frame::FlagNotRaw | Frame::FlagNotBlank);
        mapper_writer.write(frame);
    }

    LONGS_EQUAL(3, meta_writer.n_calls);

    LONGS_EQUAL(Frame::FlagNotBlank, meta_writer.flags[0]);
    LONGS_EQUAL(Frame::FlagNotBlank, meta_writer.flags[1]);
    LONGS_EQUAL(Frame::FlagNotBlank, meta_writer.flags[2]);
}

TEST(pcm_mapper_writer, flags_from_raw) {
    enum { MaxSamples = MaxBytes / sizeof(int16_t) };

    const SampleSpec in_spec(Rate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaWriter meta_writer;
    PcmMapperWriter mapper_writer(meta_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    {
        sample_t samples[MaxSamples * 3] = {};
        Frame frame(samples, MaxSamples * 3);
        frame.set_flags(Frame::FlagNotBlank);
        mapper_writer.write(frame);
    }

    LONGS_EQUAL(3, meta_writer.n_calls);

    LONGS_EQUAL(Frame::FlagNotRaw | Frame::FlagNotBlank, meta_writer.flags[0]);
    LONGS_EQUAL(Frame::FlagNotRaw | Frame::FlagNotBlank, meta_writer.flags[1]);
    LONGS_EQUAL(Frame::FlagNotRaw | Frame::FlagNotBlank, meta_writer.flags[2]);
}

TEST(pcm_mapper_writer, capture_timestamp) {
    enum { MaxSamples = MaxBytes / sizeof(sample_t) };

    const SampleSpec in_spec(Rate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaWriter meta_writer;
    PcmMapperWriter mapper_writer(meta_writer, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_writer.init_status());

    {
        sample_t samples[MaxSamples * 3] = {};
        Frame frame(samples, MaxSamples * 3);
        frame.set_capture_timestamp(1000000000);
        mapper_writer.write(frame);
    }

    LONGS_EQUAL(3, meta_writer.n_calls);

    LONGS_EQUAL(1000000000, meta_writer.cts[0]);
    LONGS_EQUAL(1010000000, meta_writer.cts[1]);
    LONGS_EQUAL(1020000000, meta_writer.cts[2]);
}

} // namespace audio
} // namespace roc
