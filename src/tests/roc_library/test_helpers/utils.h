/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_LIBRARY_TEST_HELPERS_UTILS_H_
#define ROC_LIBRARY_TEST_HELPERS_UTILS_H_

namespace roc {
namespace library {
namespace test {

namespace {

enum {
    MaxBufSize = 500,

    SampleRate = 44100,
    NumChans = 2,

    SourcePackets = 10,
    RepairPackets = 7,

    PacketSamples = 100,
    FrameSamples = PacketSamples * 2,
    TotalSamples = PacketSamples * SourcePackets * 3,

    Latency = TotalSamples / NumChans,
    Timeout = TotalSamples * 10
};

enum { FlagRS8M = (1 << 0), FlagLDPC = (1 << 1) };

inline float increment_sample_value(float sample_value, float sample_step) {
    sample_value += sample_step;
    if (sample_value + sample_step > 1.0f) {
        sample_value = sample_step;
    }
    return sample_value;
}

} // namespace

} // namespace test
} // namespace library
} // namespace roc

#endif // ROC_LIBRARY_TEST_HELPERS_UTILS_H_
