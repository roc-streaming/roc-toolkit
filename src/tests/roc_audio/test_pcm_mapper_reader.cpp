/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/pcm_format.h"
#include "roc_audio/pcm_mapper_reader.h"
#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

namespace {

const double Epsilon = 0.0001;

enum { Rate = 10000, MaxBytes = 400, SmallFrameSz = 20 };

core::HeapArena arena;
FrameFactory frame_factory(arena, MaxBytes);

template <class T> struct CountReader : IFrameReader {
    T value;
    T step;

    int n_calls;
    int n_values;

    CountReader(T step)
        : value(0)
        , step(step)
        , n_calls(0)
        , n_values(0) {
    }

    void reset() {
        value = 0;
        n_calls = 0;
        n_values = 0;
    }

    virtual status::StatusCode read(Frame& frame) {
        size_t pos = 0;
        while (pos < frame.num_bytes()) {
            *reinterpret_cast<T*>(frame.bytes() + pos) = value;
            value += step;
            pos += sizeof(T);
            n_values++;
        }
        n_calls++;
        return status::StatusOK;
    }
};

struct ByteReader : IFrameReader {
    uint8_t* buffer;
    size_t buffer_size;
    size_t buffer_pos;

    int n_calls;
    int n_bytes;

    ByteReader(uint8_t* buffer, size_t buffer_size)
        : buffer(buffer)
        , buffer_size(buffer_size)
        , buffer_pos(0)
        , n_calls(0)
        , n_bytes(0) {
    }

    virtual status::StatusCode read(Frame& frame) {
        size_t pos = 0;
        while (pos < frame.num_bytes()) {
            CHECK(buffer_pos < buffer_size);
            frame.bytes()[pos] = buffer[buffer_pos];
            pos++;
            buffer_pos++;
            n_bytes++;
        }
        n_calls++;
        return status::StatusOK;
    }
};

struct MetaReader : IFrameReader {
    unsigned flags[10];
    core::nanoseconds_t cts[10];
    unsigned pos;

    int n_calls;

    MetaReader() {
        memset(flags, 0, sizeof(flags));
        memset(cts, 0, sizeof(cts));
        pos = 0;
        n_calls = 0;
    }

    virtual status::StatusCode read(Frame& frame) {
        CHECK(pos < ROC_ARRAY_SIZE(flags));
        CHECK(pos < ROC_ARRAY_SIZE(cts));
        frame.set_flags(flags[pos]);
        frame.set_capture_timestamp(cts[pos]);
        pos++;
        n_calls++;
        return status::StatusOK;
    }
};

} // namespace

TEST_GROUP(pcm_mapper_reader) {};

TEST(pcm_mapper_reader, raw_to_raw) {
    const SampleSpec in_spec(Rate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<sample_t> count_reader(0.001f);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    sample_t samples[SmallFrameSz] = {};
    Frame frame(samples, SmallFrameSz);

    LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(SmallFrameSz, count_reader.n_values);

    for (size_t i = 0; i < SmallFrameSz; i++) {
        DOUBLES_EQUAL(i * 0.001, samples[i], Epsilon);
    }
}

TEST(pcm_mapper_reader, s16_to_raw) {
    const SampleSpec in_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<int16_t> count_reader(100);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    sample_t samples[SmallFrameSz] = {};
    Frame frame(samples, SmallFrameSz);

    LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(SmallFrameSz, count_reader.n_values);

    for (size_t i = 0; i < SmallFrameSz; i++) {
        DOUBLES_EQUAL(i * 100 / 32768., samples[i], Epsilon);
    }
}

TEST(pcm_mapper_reader, raw_to_s16) {
    const SampleSpec in_spec(Rate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<sample_t> count_reader(0.001f);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    int16_t samples[SmallFrameSz] = {};
    Frame frame((uint8_t*)samples, sizeof(samples));

    LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(SmallFrameSz, count_reader.n_values);

    for (size_t i = 0; i < SmallFrameSz; i++) {
        DOUBLES_EQUAL(i * 0.001, samples[i] / 32768., Epsilon);
    }
}

TEST(pcm_mapper_reader, s16_to_s32) {
    const SampleSpec in_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmFormat_SInt32, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<int16_t> count_reader(100);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    int32_t samples[SmallFrameSz] = {};
    Frame frame((uint8_t*)samples, sizeof(samples));

    LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(SmallFrameSz, count_reader.n_values);

    for (size_t i = 0; i < SmallFrameSz; i++) {
        DOUBLES_EQUAL(i * 100 / 32768., samples[i] / 2147483648., Epsilon);
    }
}

TEST(pcm_mapper_reader, s32_to_s16) {
    const SampleSpec in_spec(Rate, PcmFormat_SInt32, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<int32_t> count_reader(1000);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    int16_t samples[SmallFrameSz] = {};
    Frame frame((uint8_t*)samples, sizeof(samples));

    LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(SmallFrameSz, count_reader.n_values);

    for (size_t i = 0; i < SmallFrameSz; i++) {
        DOUBLES_EQUAL(i * 1000 / 2147483648., samples[i] / 32768., Epsilon);
    }
}

// Request large output frame, so that internally it's split into multiple
// smaller input frames.
TEST(pcm_mapper_reader, split_frame) {
    enum { SplitCount = 10, LargeFrameSz = MaxBytes / sizeof(int16_t) * SplitCount };

    const SampleSpec in_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<int16_t> count_reader(10);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    sample_t samples[LargeFrameSz] = {};
    Frame frame(samples, LargeFrameSz);

    LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

    LONGS_EQUAL(SplitCount, count_reader.n_calls);
    LONGS_EQUAL(LargeFrameSz, count_reader.n_values);

    for (size_t i = 0; i < LargeFrameSz; i++) {
        DOUBLES_EQUAL(i * 10 / 32768., samples[i], Epsilon);
    }
}

// Same, but repeatedly.
TEST(pcm_mapper_reader, split_frame_loop) {
    enum {
        IterCount = 20,
        SplitCount = 10,
        LargeFrameSz = MaxBytes / sizeof(int16_t) * SplitCount
    };

    const SampleSpec in_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<int16_t> count_reader(10);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    for (size_t iter = 0; iter < IterCount; iter++) {
        sample_t samples[LargeFrameSz] = {};
        Frame frame(samples, LargeFrameSz);

        count_reader.reset();
        LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

        LONGS_EQUAL(SplitCount, count_reader.n_calls);
        LONGS_EQUAL(LargeFrameSz, count_reader.n_values);

        for (size_t i = 0; i < LargeFrameSz; i++) {
            DOUBLES_EQUAL(i * 10 / 32768., samples[i], Epsilon);
        }
    }
}

TEST(pcm_mapper_reader, duration_mono) {
    const SampleSpec in_spec(Rate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    CountReader<sample_t> count_reader(0);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    sample_t samples[SmallFrameSz] = {};
    Frame frame(samples, SmallFrameSz);

    LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(SmallFrameSz, count_reader.n_values);

    LONGS_EQUAL(SmallFrameSz, frame.duration());
}

TEST(pcm_mapper_reader, duration_stereo) {
    const SampleSpec in_spec(Rate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Stereo);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Stereo);

    CountReader<sample_t> count_reader(0);
    PcmMapperReader mapper_reader(count_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    sample_t samples[SmallFrameSz] = {};
    Frame frame(samples, SmallFrameSz);

    LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

    LONGS_EQUAL(1, count_reader.n_calls);
    LONGS_EQUAL(SmallFrameSz, count_reader.n_values);

    LONGS_EQUAL(SmallFrameSz / 2, frame.duration());
}

TEST(pcm_mapper_reader, flags_to_raw) {
    enum { MaxSamples = MaxBytes / sizeof(int16_t) };

    const SampleSpec in_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaReader meta_reader;
    PcmMapperReader mapper_reader(meta_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    meta_reader.flags[0] = Frame::FlagNotRaw;
    meta_reader.flags[1] = Frame::FlagNotRaw | Frame::FlagNotBlank;
    meta_reader.flags[2] = Frame::FlagNotRaw | Frame::FlagNotComplete;

    {
        sample_t samples[MaxSamples * 3] = {};
        Frame frame(samples, MaxSamples * 3);

        LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

        LONGS_EQUAL(3, meta_reader.n_calls);

        UNSIGNED_LONGS_EQUAL(Frame::FlagNotBlank | Frame::FlagNotComplete, frame.flags());
    }

    meta_reader.flags[3] = Frame::FlagNotRaw;
    meta_reader.flags[4] =
        Frame::FlagNotRaw | Frame::FlagNotBlank | Frame::FlagPacketDrops;
    meta_reader.flags[5] = Frame::FlagNotRaw;

    {
        sample_t samples[MaxSamples * 3] = {};
        Frame frame(samples, MaxSamples * 3);

        LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

        LONGS_EQUAL(6, meta_reader.n_calls);

        UNSIGNED_LONGS_EQUAL(Frame::FlagNotBlank | Frame::FlagPacketDrops, frame.flags());
    }
}

TEST(pcm_mapper_reader, flags_from_raw) {
    enum { MaxSamples = MaxBytes / sizeof(sample_t) };

    const SampleSpec in_spec(Rate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, PcmFormat_SInt16, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaReader meta_reader;
    PcmMapperReader mapper_reader(meta_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    meta_reader.flags[0] = 0;
    meta_reader.flags[1] = Frame::FlagNotBlank;
    meta_reader.flags[2] = Frame::FlagNotComplete;

    {
        int16_t samples[MaxSamples * 3] = {};
        Frame frame((uint8_t*)samples, sizeof(samples));

        LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

        LONGS_EQUAL(3, meta_reader.n_calls);

        UNSIGNED_LONGS_EQUAL(Frame::FlagNotRaw | Frame::FlagNotBlank
                                 | Frame::FlagNotComplete,
                             frame.flags());
    }

    meta_reader.flags[3] = 0;
    meta_reader.flags[4] = Frame::FlagNotBlank | Frame::FlagPacketDrops;
    meta_reader.flags[5] = 0;

    {
        int16_t samples[MaxSamples * 3] = {};
        Frame frame((uint8_t*)samples, sizeof(samples));

        LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

        LONGS_EQUAL(6, meta_reader.n_calls);

        UNSIGNED_LONGS_EQUAL(Frame::FlagNotRaw | Frame::FlagNotBlank
                                 | Frame::FlagPacketDrops,
                             frame.flags());
    }
}

TEST(pcm_mapper_reader, capture_timestamp) {
    enum { MaxSamples = MaxBytes / sizeof(sample_t) };

    const SampleSpec in_spec(Rate, Sample_RawFormat, ChanLayout_Surround, ChanOrder_Smpte,
                             ChanMask_Surround_Mono);
    const SampleSpec out_spec(Rate, Sample_RawFormat, ChanLayout_Surround,
                              ChanOrder_Smpte, ChanMask_Surround_Mono);

    MetaReader meta_reader;
    PcmMapperReader mapper_reader(meta_reader, frame_factory, in_spec, out_spec);
    LONGS_EQUAL(status::StatusOK, mapper_reader.init_status());

    meta_reader.cts[0] = 10000000000;
    meta_reader.cts[1] = 20000000000;
    meta_reader.cts[2] = 30000000000;

    {
        sample_t samples[MaxSamples * 3] = {};
        Frame frame(samples, MaxSamples * 3);

        LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

        LONGS_EQUAL(3, meta_reader.n_calls);

        LONGLONGS_EQUAL(10000000000, frame.capture_timestamp());
    }

    meta_reader.cts[3] = 40000000000;
    meta_reader.cts[4] = 50000000000;
    meta_reader.cts[5] = 60000000000;

    {
        sample_t samples[MaxSamples * 3] = {};
        Frame frame(samples, MaxSamples * 3);

        LONGS_EQUAL(status::StatusOK, mapper_reader.read(frame));

        LONGS_EQUAL(6, meta_reader.n_calls);

        LONGLONGS_EQUAL(40000000000, frame.capture_timestamp());
    }
}

} // namespace audio
} // namespace roc
