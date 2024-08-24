/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/freq_estimator.h"
#include "roc_core/macro_helpers.h"

namespace roc {
namespace audio {

namespace {

enum { Target = 10000 };

const double Epsilon = 0.0001;

const LatencyTunerProfile profile_list[] = { LatencyTunerProfile_Responsive,
                                             LatencyTunerProfile_Gradual };

const SampleSpec sample_spec(44100,
                             PcmSubformat_Raw,
                             ChanLayout_Surround,
                             ChanOrder_Smpte,
                             ChanMask_Surround_Mono);

} // namespace

TEST_GROUP(freq_estimator) {};

TEST(freq_estimator, initial) {
    for (size_t p = 0; p < ROC_ARRAY_SIZE(profile_list); p++) {
        FreqEstimatorConfig config;
        CHECK(config.deduce_defaults(profile_list[p]));

        FreqEstimator fe(config, Target, sample_spec, NULL);

        DOUBLES_EQUAL(1.0, (double)fe.freq_coeff(), Epsilon);
    }
}

TEST(freq_estimator, aim_queue_size) {
    for (size_t p = 0; p < ROC_ARRAY_SIZE(profile_list); p++) {
        FreqEstimatorConfig config;
        CHECK(config.deduce_defaults(profile_list[p]));

        FreqEstimator fe(config, Target, sample_spec, NULL);

        for (size_t n = 0; n < 1000; n++) {
            fe.update_current_latency(Target);
        }

        DOUBLES_EQUAL(1.0, (double)fe.freq_coeff(), Epsilon);
    }
}

TEST(freq_estimator, large_queue_size) {
    for (size_t p = 0; p < ROC_ARRAY_SIZE(profile_list); p++) {
        FreqEstimatorConfig config;
        CHECK(config.deduce_defaults(profile_list[p]));

        FreqEstimator fe(config, Target, sample_spec, NULL);

        do {
            fe.update_current_latency(Target * 2);
        } while (fe.freq_coeff() < 1.01f);
    }
}

TEST(freq_estimator, small_queue_size) {
    for (size_t p = 0; p < ROC_ARRAY_SIZE(profile_list); p++) {
        FreqEstimatorConfig config;
        CHECK(config.deduce_defaults(profile_list[p]));

        FreqEstimator fe(config, Target, sample_spec, NULL);

        do {
            fe.update_current_latency(Target / 2);
        } while (fe.freq_coeff() > 0.997f);
    }
}

} // namespace audio
} // namespace roc
