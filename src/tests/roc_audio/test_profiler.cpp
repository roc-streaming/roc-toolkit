/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/profiler.h"
#include "roc_core/heap_arena.h"

namespace roc {
namespace audio {

namespace {
struct TestFrame {
    TestFrame(size_t sz, core::nanoseconds_t tm)
        : size(sz)
        , time(tm) {
    }

    size_t size;
    core::nanoseconds_t time;
};

const double EpsilionThreshold = 0.001;

const int SampleRate = 5000; // 50 samples / chunk
const int ChannelMask = 0x1;

const SampleSpec sample_spec(
    SampleRate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte, ChannelMask);

const ProfilerConfig profiler_config(50 * core::Millisecond, 10 * core::Millisecond);

core::HeapArena arena;

} // namespace

TEST_GROUP(profiler) {};

TEST(profiler, moving_average) {
    Profiler profiler(arena, sample_spec, profiler_config);
    LONGS_EQUAL(status::StatusOK, profiler.init_status());

    TestFrame frames[] = {
        TestFrame(50, 50 * core::Second),      TestFrame(25, 25 * core::Second),
        TestFrame(25, 25 * core::Second),      TestFrame(25, 25 * core::Second),
        TestFrame(25, 25 * core::Second / 2),  TestFrame(40, 40 * core::Second),
        TestFrame(60, 60 * core::Second / 3),  TestFrame(50, 50 * core::Second),
        TestFrame(125, 125 * core::Second / 3)
    };

    // populate frame speeds
    double frame_speeds[ROC_ARRAY_SIZE(frames)];
    for (size_t i = 0; i < ROC_ARRAY_SIZE(frames); ++i) {
        frame_speeds[i] = double(frames[i].size * core::Second) / frames[i].time
            / sample_spec.num_channels();
    }

    size_t samples_in_moving_avg = 0;
    double expected_average[ROC_ARRAY_SIZE(frames)];
    expected_average[0] = (frame_speeds[0]) / 1;
    samples_in_moving_avg += frames[0].size; // 50
    // 2nd chunk not full
    expected_average[1] = (((frame_speeds[0]) / 1) * samples_in_moving_avg
                           + frame_speeds[1] * frames[1].size)
        / (samples_in_moving_avg + frames[1].size);
    samples_in_moving_avg += frames[1].size; // 75
    // second chunk populated
    expected_average[2] =
        (frame_speeds[0] + (0.5 * frame_speeds[1] + 0.5 * frame_speeds[2])) / 2;
    samples_in_moving_avg += frames[2].size; // 100
    // 3rd chunk not populated
    expected_average[3] =
        (((frame_speeds[0] + (0.5 * frame_speeds[1] + 0.5 * frame_speeds[2])) / 2)
             * samples_in_moving_avg
         + frame_speeds[3] * frames[3].size)
        / (samples_in_moving_avg + frames[3].size);
    samples_in_moving_avg += frames[3].size; // 125
    // 3rd chunk full
    expected_average[4] =
        (frame_speeds[0] + (0.5 * frame_speeds[1] + 0.5 * frame_speeds[2])
         + (0.5 * frame_speeds[3] + 0.5 * frame_speeds[4]))
        / 3;
    samples_in_moving_avg += frames[4].size; // 150
    // 4th chunk not fully populated
    expected_average[5] =
        (((frame_speeds[0] + (0.5 * frame_speeds[1] + 0.5 * frame_speeds[2])
           + (0.5 * frame_speeds[3] + 0.5 * frame_speeds[4]))
          / 3)
             * samples_in_moving_avg
         + frame_speeds[5] * frames[5].size)
        / (samples_in_moving_avg + frames[5].size);
    samples_in_moving_avg += frames[5].size; // 190

    // 4th and 5th chunk filled
    expected_average[6] =
        (frame_speeds[0] + (0.5 * frame_speeds[1] + 0.5 * frame_speeds[2])
         + (0.5 * frame_speeds[3] + 0.5 * frame_speeds[4])
         + (0.8 * frame_speeds[5] + 0.2 * frame_speeds[6]) + frame_speeds[6])
        / 5;
    samples_in_moving_avg += frames[6].size; // 250

    // 1st chunk overwritten
    expected_average[7] = ((0.5 * frame_speeds[1] + 0.5 * frame_speeds[2])
                           + (0.5 * frame_speeds[3] + 0.5 * frame_speeds[4])
                           + (0.8 * frame_speeds[5] + 0.2 * frame_speeds[6])
                           + frame_speeds[6] + frame_speeds[7])
        / 5;

    // 2nd and 3rd chunk overwritten 4th partially populated
    expected_average[8] =
        ((((0.8 * frame_speeds[5] + 0.2 * frame_speeds[6]) + frame_speeds[6]
           + frame_speeds[7] + frame_speeds[8] * 2)
          / 5)
             * samples_in_moving_avg
         - (0.8 * frame_speeds[5] + 0.2 * frame_speeds[6]) * (frames[8].size - 100)
         + frame_speeds[8] * (frames[8].size - 100))
        / samples_in_moving_avg;

    for (size_t i = 0; i < ROC_ARRAY_SIZE(frames); ++i) {
        profiler.add_frame((packet::stream_timestamp_t)frames[i].size
                               / sample_spec.num_channels(),
                           frames[i].time);
        DOUBLES_EQUAL(expected_average[i], profiler.get_moving_avg(), EpsilionThreshold);
    }
}

} // namespace audio
} // namespace roc
