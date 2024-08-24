/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PUBLIC_API_TEST_HELPERS_UTILS_H_
#define ROC_PUBLIC_API_TEST_HELPERS_UTILS_H_

namespace roc {
namespace api {
namespace test {

namespace {

enum {
    MaxBufSize = 5120,

    SourcePackets = 10,
    RepairPackets = 7,

    PacketSamples = 15,
    FrameSamples = PacketSamples * 2,
    TotalSamples = PacketSamples * SourcePackets * 3,

    Latency = TotalSamples,
    Timeout = TotalSamples * 10
};

enum {
    FlagRS8M = (1 << 0),
    FlagLDPC = (1 << 1),
    FlagRTCP = (1 << 2),
    FlagMultitrack = (1 << 3),
    FlagNonStrict = (1 << 4),
    FlagInfinite = (1 << 5),
    FlagLoseSomePkts = (1 << 6),
    FlagLoseAllRepairPkts = (1 << 7),
};

inline float increment_sample_value(float sample_value, float sample_step) {
    sample_value += sample_step;
    if (sample_value + sample_step > 1.0f) {
        sample_value = sample_step;
    }
    return sample_value;
}

} // namespace

} // namespace test
} // namespace api
} // namespace roc

#endif // ROC_PUBLIC_API_TEST_HELPERS_UTILS_H_
