/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_mock_source.h"

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/stddefs.h"
#include "roc_core/temp_file.h"
#include "roc_sndio/pump.h"
#include "roc_sndio/sox_sink.h"
#include "roc_sndio/sox_source.h"

namespace roc {
namespace sndio {

namespace {

enum {
    MaxBufSize = 8192,
    FrameSize = 500,
    SampleRate = 44100,
    ChMask = 0x3,
    NumChans = 2
};

const core::nanoseconds_t FrameDuration =
    FrameSize * core::Second / (SampleRate * packet::num_channels(ChMask));

core::HeapAllocator allocator;
core::BufferPool<audio::sample_t> buffer_pool(allocator, MaxBufSize, true);

} // namespace

TEST_GROUP(sox_source) {
    Config sink_config;
    Config source_config;

    void setup() {
        sink_config.channels = ChMask;
        sink_config.sample_rate = SampleRate;
        sink_config.frame_length = FrameDuration;

        source_config.channels = ChMask;
        source_config.sample_rate = SampleRate;
        source_config.frame_length = FrameDuration;
    }
};

TEST(sox_source, noop) {
    SoxSource sox_source(allocator, source_config);
}

TEST(sox_source, error) {
    SoxSource sox_source(allocator, source_config);

    CHECK(!sox_source.open(NULL, "/bad/file"));
}

TEST(sox_source, has_clock) {
    core::TempFile file("test.wav");

    {
        MockSource mock_source;
        mock_source.add(MaxBufSize * 10);

        SoxSink sox_sink(allocator, sink_config);
        CHECK(sox_sink.open(NULL, file.path()));

        Pump pump(buffer_pool, mock_source, NULL, sox_sink, FrameDuration, SampleRate,
                  ChMask, Pump::ModeOneshot);
        CHECK(pump.valid());
        CHECK(pump.run());
    }

    SoxSource sox_source(allocator, source_config);

    CHECK(sox_source.open(NULL, file.path()));
    CHECK(!sox_source.has_clock());
}

TEST(sox_source, sample_rate_auto) {
    core::TempFile file("test.wav");

    {
        MockSource mock_source;
        mock_source.add(MaxBufSize * 10);

        SoxSink sox_sink(allocator, sink_config);
        CHECK(sox_sink.open(NULL, file.path()));

        Pump pump(buffer_pool, mock_source, NULL, sox_sink, FrameDuration, SampleRate,
                  ChMask, Pump::ModeOneshot);
        CHECK(pump.valid());
        CHECK(pump.run());
    }

    source_config.sample_rate = 0;
    source_config.frame_length = FrameDuration;
    SoxSource sox_source(allocator, source_config);

    CHECK(sox_source.open(NULL, file.path()));
    CHECK(sox_source.sample_rate() == SampleRate);
}

TEST(sox_source, sample_rate_mismatch) {
    core::TempFile file("test.wav");

    {
        MockSource mock_source;
        mock_source.add(MaxBufSize * 10);

        SoxSink sox_sink(allocator, sink_config);
        CHECK(sox_sink.open(NULL, file.path()));

        Pump pump(buffer_pool, mock_source, NULL, sox_sink, FrameDuration, SampleRate,
                  ChMask, Pump::ModeOneshot);
        CHECK(pump.valid());
        CHECK(pump.run());
    }

    source_config.sample_rate = SampleRate * 2;
    SoxSource sox_source(allocator, source_config);

    CHECK(sox_source.open(NULL, file.path()));
    CHECK(sox_source.sample_rate() == SampleRate * 2);
}

TEST(sox_source, pause_resume) {
    core::TempFile file("test.wav");

    {
        MockSource mock_source;
        mock_source.add(FrameSize * NumChans * 2);

        SoxSink sox_sink(allocator, sink_config);
        CHECK(sox_sink.open(NULL, file.path()));

        Pump pump(buffer_pool, mock_source, NULL, sox_sink, FrameDuration, SampleRate,
                  ChMask, Pump::ModeOneshot);
        CHECK(pump.valid());
        CHECK(pump.run());
    }

    SoxSource sox_source(allocator, source_config);

    CHECK(sox_source.open(NULL, file.path()));

    audio::sample_t frame_data1[FrameSize * NumChans] = {};
    audio::Frame frame1(frame_data1, FrameSize * NumChans);

    CHECK(sox_source.state() == ISource::Playing);
    CHECK(sox_source.read(frame1));

    sox_source.pause();
    CHECK(sox_source.state() == ISource::Paused);

    audio::sample_t frame_data2[FrameSize * NumChans] = {};
    audio::Frame frame2(frame_data2, FrameSize * NumChans);

    CHECK(!sox_source.read(frame2));

    CHECK(sox_source.resume());
    CHECK(sox_source.state() == ISource::Playing);

    CHECK(sox_source.read(frame2));

    if (memcmp(frame_data1, frame_data2, sizeof(frame_data1)) == 0) {
        FAIL("frames should not be equal");
    }
}

TEST(sox_source, pause_restart) {
    core::TempFile file("test.wav");

    {
        MockSource mock_source;
        mock_source.add(FrameSize * NumChans * 2);

        SoxSink sox_sink(allocator, sink_config);
        CHECK(sox_sink.open(NULL, file.path()));

        Pump pump(buffer_pool, mock_source, NULL, sox_sink, FrameDuration, SampleRate,
                  ChMask, Pump::ModeOneshot);
        CHECK(pump.valid());
        CHECK(pump.run());
    }

    SoxSource sox_source(allocator, source_config);

    CHECK(sox_source.open(NULL, file.path()));

    audio::sample_t frame_data1[FrameSize * NumChans] = {};
    audio::Frame frame1(frame_data1, FrameSize * NumChans);

    CHECK(sox_source.state() == ISource::Playing);
    CHECK(sox_source.read(frame1));

    sox_source.pause();
    CHECK(sox_source.state() == ISource::Paused);

    audio::sample_t frame_data2[FrameSize * NumChans] = {};
    audio::Frame frame2(frame_data2, FrameSize * NumChans);

    CHECK(!sox_source.read(frame2));

    CHECK(sox_source.restart());
    CHECK(sox_source.state() == ISource::Playing);

    CHECK(sox_source.read(frame2));

    if (memcmp(frame_data1, frame_data2, sizeof(frame_data1)) != 0) {
        FAIL("frames should be equal");
    }
}

TEST(sox_source, eof_restart) {
    core::TempFile file("test.wav");
    {
        MockSource mock_source;
        mock_source.add(FrameSize * NumChans * 2);

        SoxSink sox_sink(allocator, sink_config);
        CHECK(sox_sink.open(NULL, file.path()));

        Pump pump(buffer_pool, mock_source, NULL, sox_sink, FrameDuration, SampleRate,
                  ChMask, Pump::ModeOneshot);
        CHECK(pump.valid());
        CHECK(pump.run());
    }

    SoxSource sox_source(allocator, source_config);

    CHECK(sox_source.open(NULL, file.path()));

    audio::sample_t frame_data[FrameSize * NumChans] = {};
    audio::Frame frame(frame_data, FrameSize * NumChans);

    for (int i = 0; i < 3; i++) {
        CHECK(sox_source.read(frame));
        CHECK(sox_source.read(frame));
        CHECK(!sox_source.read(frame));

        CHECK(sox_source.restart());
    }
}

} // namespace sndio
} // namespace roc
