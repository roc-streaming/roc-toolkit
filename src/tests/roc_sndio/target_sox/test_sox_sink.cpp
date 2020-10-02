/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/sample_spec.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/temp_file.h"
#include "roc_sndio/sox_sink.h"

namespace roc {
namespace sndio {

namespace {

enum { FrameSize = 500, SampleRate = 44100, ChMask = 0x3 };

const audio::SampleSpec sample_spec = audio::SampleSpec(SampleRate, ChMask);
const core::nanoseconds_t FrameDuration =
    FrameSize * core::Second / (SampleRate * sample_spec.num_channels());

core::HeapAllocator allocator;

} // namespace

TEST_GROUP(sox_sink) {
    Config sink_config;

    void setup() {
        sink_config.sample_spec = audio::SampleSpec(SampleRate, ChMask);
        sink_config.frame_length = FrameDuration;
    }
};

TEST(sox_sink, noop) {
    SoxSink sox_sink(allocator, sink_config);
}

TEST(sox_sink, error) {
    SoxSink sox_sink(allocator, sink_config);

    CHECK(!sox_sink.open(NULL, "/bad/file"));
}

TEST(sox_sink, has_clock) {
    SoxSink sox_sink(allocator, sink_config);

    core::TempFile file("test.wav");
    CHECK(sox_sink.open(NULL, file.path()));
    CHECK(!sox_sink.has_clock());
}

TEST(sox_sink, sample_rate_auto) {
    sink_config.sample_spec.setSampleRate(0);
    sink_config.frame_length = FrameDuration;
    SoxSink sox_sink(allocator, sink_config);

    core::TempFile file("test.wav");
    CHECK(sox_sink.open(NULL, file.path()));
    CHECK(sox_sink.sample_rate() != 0);
}

TEST(sox_sink, sample_rate_force) {
    sink_config.sample_spec.setSampleRate(SampleRate);
    SoxSink sox_sink(allocator, sink_config);

    core::TempFile file("test.wav");
    CHECK(sox_sink.open(NULL, file.path()));
    CHECK(sox_sink.sample_rate() == SampleRate);
}

} // namespace sndio
} // namespace roc
