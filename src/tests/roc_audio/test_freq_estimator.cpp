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

const FreqEstimatorProfile Profiles[] = { FreqEstimatorProfile_Responsive,
                                          FreqEstimatorProfile_Gradual };

const double Epsilon = 0.0001;

} // namespace

TEST_GROUP(freq_estimator) {
    FreqEstimatorConfig fe_config;
};

TEST(freq_estimator, initial) {
    for (size_t p = 0; p < ROC_ARRAY_SIZE(Profiles); p++) {
        FreqEstimator fe(Profiles[p], Target, NULL);

        DOUBLES_EQUAL(1.0, (double)fe.freq_coeff(), Epsilon);
    }
}

TEST(freq_estimator, aim_queue_size) {
    for (size_t p = 0; p < ROC_ARRAY_SIZE(Profiles); p++) {
        FreqEstimator fe(Profiles[p], Target, NULL);

        for (size_t n = 0; n < 1000; n++) {
            fe.update_current_latency(Target);
        }

        DOUBLES_EQUAL(1.0, (double)fe.freq_coeff(), Epsilon);
    }
}

TEST(freq_estimator, large_queue_size) {
    for (size_t p = 0; p < ROC_ARRAY_SIZE(Profiles); p++) {
        FreqEstimator fe(Profiles[p], Target, NULL);

        do {
            fe.update_current_latency(Target * 2);
        } while (fe.freq_coeff() < 1.01f);
    }
}

TEST(freq_estimator, small_queue_size) {
    for (size_t p = 0; p < ROC_ARRAY_SIZE(Profiles); p++) {
        FreqEstimator fe(Profiles[p], Target, NULL);

        do {
            fe.update_current_latency(Target / 2);
        } while (fe.freq_coeff() > 0.997f);
    }
}

} // namespace audio
} // namespace roc
