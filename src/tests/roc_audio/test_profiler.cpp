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

const core::nanoseconds_t interval = 1000;
const int sample_rate = 300;
const int num_channels = 1;
core::HeapAllocator allocator;

} // namespace

TEST_GROUP(profiler) {};

TEST(profiler, test_moving_average) {
    Profiler profiler(allocator, num_channels, sample_rate, interval);

    size_t samples = 0;
    core::nanoseconds_t time = 0;
    TestFrame frames[3] = { TestFrame(50, 200), TestFrame(100, 100),
                            TestFrame(500, 700) };

    // test up till 1000ns
    for (int i = 0; i < 3; ++i) {
        samples += frames[i].size_;
        time += frames[i].time_;
        profiler.end_frame(frames[i].size_, frames[i].time_);
        LONGS_EQUAL((int)samples * core::Second / time, profiler.get_moving_avg());
    }

    // we have reached 1000ns of data
    // test minimal removal from 1 node
    // add 100 samples in 100ns from new data
    // remove 25 samples from front 100ns of data
    size_t frame_size = 100;
    core::nanoseconds_t frame_time = 100;
    samples += frame_size;
    size_t frames_to_remove =
        (frames[0].size_ * (unsigned long)frame_time / (unsigned long)frames[0].time_);
    samples -= frames_to_remove;
    frames[0].size_ -= frames_to_remove;
    frames[0].time_ -= frame_time;
    profiler.end_frame(frame_size, frame_time);
    LONGS_EQUAL((int)samples * core::Second / time, profiler.get_moving_avg());

    // test larger removal from 2 nodes of samples
    // add 50 samples in 200ns from new data
    frame_size = 50;
    frame_time = 200;
    samples += frame_size;
    // remove 25 samples and 100ns from node0 and remove 100 samples and 100ns from node1
    samples -= (frames[0].size_
                + frames[1].size_ * (unsigned long)(frame_time - frames[0].time_)
                    / (unsigned long)(frames[1].time_));
    profiler.end_frame(frame_size, frame_time);
    LONGS_EQUAL((int)samples * core::Second / time, profiler.get_moving_avg());
}

} // namespace audio
} // namespace roc
