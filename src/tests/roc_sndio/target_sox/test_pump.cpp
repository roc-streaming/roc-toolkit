/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_mock_sink.h"
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

enum { MaxBufSize = 8192, SampleRate = 44100, ChMask = 0x3 };

const core::nanoseconds_t MaxBufDuration =
    MaxBufSize * core::Second / (SampleRate * packet::num_channels(ChMask));

core::HeapAllocator allocator;
core::BufferPool<audio::sample_t> buffer_pool(allocator, MaxBufSize, true);

} // namespace

TEST_GROUP(pump) {
    Config config;

    void setup() {
        config.channels = ChMask;
        config.sample_rate = SampleRate;
        config.frame_length = MaxBufDuration;
    }
};

TEST(pump, write_read) {
    enum { NumSamples = MaxBufSize * 10 };

    MockSource mock_source;
    mock_source.add(NumSamples);

    core::TempFile file("test.wav");

    {
        SoxSink sox_sink(allocator, config);
        CHECK(sox_sink.open(NULL, file.path()));

        Pump pump(buffer_pool, mock_source, NULL, sox_sink, MaxBufDuration, SampleRate,
                  ChMask, Pump::ModeOneshot);
        CHECK(pump.valid());
        CHECK(pump.run());

        CHECK(mock_source.num_returned() >= NumSamples - MaxBufSize);
    }

    SoxSource sox_source(allocator, config);
    CHECK(sox_source.open(NULL, file.path()));

    MockSink mock_writer;

    Pump pump(buffer_pool, sox_source, NULL, mock_writer, MaxBufDuration, SampleRate,
              ChMask, Pump::ModePermanent);
    CHECK(pump.valid());
    CHECK(pump.run());

    mock_writer.check(0, mock_source.num_returned());
}

TEST(pump, write_overwrite_read) {
    enum { NumSamples = MaxBufSize * 10 };

    MockSource mock_source;
    mock_source.add(NumSamples);

    core::TempFile file("test.wav");

    {
        SoxSink sox_sink(allocator, config);
        CHECK(sox_sink.open(NULL, file.path()));

        Pump pump(buffer_pool, mock_source, NULL, sox_sink, MaxBufDuration, SampleRate,
                  ChMask, Pump::ModeOneshot);
        CHECK(pump.valid());
        CHECK(pump.run());
    }

    mock_source.add(NumSamples);

    size_t num_returned1 = mock_source.num_returned();
    CHECK(num_returned1 >= NumSamples - MaxBufSize);

    {
        SoxSink sox_sink(allocator, config);
        CHECK(sox_sink.open(NULL, file.path()));

        Pump pump(buffer_pool, mock_source, NULL, sox_sink, MaxBufDuration, SampleRate,
                  ChMask, Pump::ModeOneshot);
        CHECK(pump.valid());
        CHECK(pump.run());
    }

    size_t num_returned2 = mock_source.num_returned() - num_returned1;
    CHECK(num_returned1 >= NumSamples - MaxBufSize);

    SoxSource sox_source(allocator, config);
    CHECK(sox_source.open(NULL, file.path()));

    MockSink mock_writer;

    Pump pump(buffer_pool, sox_source, NULL, mock_writer, MaxBufDuration, SampleRate,
              ChMask, Pump::ModePermanent);
    CHECK(pump.valid());
    CHECK(pump.run());

    mock_writer.check(num_returned1, num_returned2);
}

} // namespace sndio
} // namespace roc
