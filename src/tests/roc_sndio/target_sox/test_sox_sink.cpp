/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_allocator.h"
#include "roc_core/temp_file.h"
#include "roc_sndio/sox_sink.h"

namespace roc {
namespace sndio {

namespace {

enum { FrameSize = 512, SampleRate = 44100, ChMask = 0x3 };

core::HeapAllocator allocator;

} // namespace

TEST_GROUP(sox_sink){};

TEST(sox_sink, noop) {
    SoxSink sox_sink(allocator, ChMask, SampleRate, FrameSize);
}

TEST(sox_sink, error) {
    SoxSink sox_sink(allocator, ChMask, SampleRate, FrameSize);

    CHECK(!sox_sink.open(NULL, "/bad/file"));
}

TEST(sox_sink, is_file) {
    SoxSink sox_sink(allocator, ChMask, SampleRate, FrameSize);

    core::TempFile file("test.wav");
    CHECK(sox_sink.open(NULL, file.path()));
    CHECK(sox_sink.is_file());
}

TEST(sox_sink, sample_rate_auto) {
    SoxSink sox_sink(allocator, ChMask, 0, FrameSize);

    core::TempFile file("test.wav");
    CHECK(sox_sink.open(NULL, file.path()));
    CHECK(sox_sink.sample_rate() != 0);
}

TEST(sox_sink, sample_rate_force) {
    SoxSink sox_sink(allocator, ChMask, SampleRate, FrameSize);

    core::TempFile file("test.wav");
    CHECK(sox_sink.open(NULL, file.path()));
    CHECK(sox_sink.sample_rate() == SampleRate);
}

} // namespace sndio
} // namespace roc
