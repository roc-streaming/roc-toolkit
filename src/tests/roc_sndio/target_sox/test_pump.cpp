/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_sink.h"
#include "test_helpers/mock_source.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/stddefs.h"
#include "roc_core/temp_file.h"
#include "roc_sndio/pump.h"
#include "roc_sndio/sox_sink.h"
#include "roc_sndio/sox_source.h"

namespace roc {
namespace sndio {

namespace {

enum { BufSize = 512, SampleRate = 44100, ChMask = 0x3 };

const audio::SampleSpec
    SampleSpecs(SampleRate, audio::ChanLayout_Surround, audio::ChanOrder_Smpte, ChMask);

const core::nanoseconds_t BufDuration = BufSize * core::Second
    / core::nanoseconds_t(SampleSpecs.sample_rate() * SampleSpecs.num_channels());

core::HeapArena arena;
core::BufferFactory<audio::sample_t> buffer_factory(arena, BufSize);

} // namespace

TEST_GROUP(pump) {
    Config config;

    void setup() {
        config.sample_spec = audio::SampleSpec(SampleRate, audio::ChanLayout_Surround,
                                               audio::ChanOrder_Smpte, ChMask);
        config.frame_length = BufDuration;
    }
};

TEST(pump, write_read) {
    enum { NumSamples = BufSize * 10 };

    test::MockSource mock_source;
    mock_source.add(NumSamples);

    core::TempFile file("test.wav");

    {
        SoxSink sox_sink(arena, config);
        CHECK(sox_sink.open(NULL, file.path()));

        Pump pump(buffer_factory, mock_source, NULL, sox_sink, BufDuration, SampleSpecs,
                  Pump::ModeOneshot);
        CHECK(pump.is_valid());
        CHECK(pump.run());

        CHECK(mock_source.num_returned() >= NumSamples - BufSize);
    }

    SoxSource sox_source(arena, config);
    CHECK(sox_source.open(NULL, file.path()));

    test::MockSink mock_writer;

    Pump pump(buffer_factory, sox_source, NULL, mock_writer, BufDuration, SampleSpecs,
              Pump::ModePermanent);
    CHECK(pump.is_valid());
    CHECK(pump.run());

    mock_writer.check(0, mock_source.num_returned());
}

TEST(pump, write_overwrite_read) {
    enum { NumSamples = BufSize * 10 };

    test::MockSource mock_source;
    mock_source.add(NumSamples);

    core::TempFile file("test.wav");

    {
        SoxSink sox_sink(arena, config);
        CHECK(sox_sink.open(NULL, file.path()));

        Pump pump(buffer_factory, mock_source, NULL, sox_sink, BufDuration, SampleSpecs,
                  Pump::ModeOneshot);
        CHECK(pump.is_valid());
        CHECK(pump.run());
    }

    mock_source.add(NumSamples);

    size_t num_returned1 = mock_source.num_returned();
    CHECK(num_returned1 >= NumSamples - BufSize);

    {
        SoxSink sox_sink(arena, config);
        CHECK(sox_sink.open(NULL, file.path()));

        Pump pump(buffer_factory, mock_source, NULL, sox_sink, BufDuration, SampleSpecs,
                  Pump::ModeOneshot);
        CHECK(pump.is_valid());
        CHECK(pump.run());
    }

    size_t num_returned2 = mock_source.num_returned() - num_returned1;
    CHECK(num_returned1 >= NumSamples - BufSize);

    SoxSource sox_source(arena, config);
    CHECK(sox_source.open(NULL, file.path()));

    test::MockSink mock_writer;

    Pump pump(buffer_factory, sox_source, NULL, mock_writer, BufDuration, SampleSpecs,
              Pump::ModePermanent);
    CHECK(pump.is_valid());
    CHECK(pump.run());

    mock_writer.check(num_returned1, num_returned2);
}

} // namespace sndio
} // namespace roc
