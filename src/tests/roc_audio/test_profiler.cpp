/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/profiler.h"
#include "roc_core/heap_allocator.h"

namespace roc {
namespace audio {

namespace {

struct TestFrame {
    TestFrame(size_t size, core::nanoseconds_t time)
        : size_(size)
        , time_(time) {
    }

    size_t size_;
    core::nanoseconds_t time_;
};

int millisecond = 1000000L;
const core::nanoseconds_t interval = 50 * millisecond; // 5 chunks
const int sample_rate = 5000;                          // 50 samples / chunk
const int num_channels = 1;
core::HeapAllocator allocator;

} // namespace

TEST_GROUP(profiler) {};

TEST(profiler, test_moving_average) {
    Profiler profiler(allocator, num_channels, sample_rate, interval);

    TestFrame frames[9] = {
        TestFrame(50, 50 * core::Second),      TestFrame(25, 25 * core::Second),
        TestFrame(25, 25 * core::Second),      TestFrame(25, 25 * core::Second),
        TestFrame(25, 25 * core::Second / 2),  TestFrame(40, 40 * core::Second),
        TestFrame(60, 60 * core::Second / 3),  TestFrame(50, 50 * core::Second),
        TestFrame(125, 125 * core::Second / 3)
    };

    // hand calculated values
    double expected_moving_average[9] = { 1.000, 1.000, 1.000, 1.000, 1.167,
                                          1.167, 1.580, 1.580, 2.280 };

    for (int i = 0; i < 9; ++i) {
        profiler.end_frame(frames[i].size_, frames[i].time_);
        LONGS_EQUAL(expected_moving_average[i], profiler.get_moving_avg());
    }
}

} // namespace audio
} // namespace roc
