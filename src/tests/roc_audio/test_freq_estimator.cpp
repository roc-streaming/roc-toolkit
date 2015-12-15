/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_config/config.h"

#include "roc_audio/freq_estimator.h"

namespace roc {
namespace test {

using namespace audio;

namespace {

const size_t FE_AIM = ROC_CONFIG_DEFAULT_RENDERER_LATENCY;

const size_t FE_N_ITERATIONS = 1000;

const double EPSILON = 1e-6;

} // namespace

TEST_GROUP(freq_estimator) {
    FreqEstimator fe;
};

TEST(freq_estimator, initial) {
    DOUBLES_EQUAL(1.0, fe.freq_coeff(), EPSILON);
}

TEST(freq_estimator, aim_queue_size) {
    for (size_t n = 0; n < FE_N_ITERATIONS; n++) {
        fe.update(FE_AIM);
    }

    DOUBLES_EQUAL(1.0, fe.freq_coeff(), EPSILON);
}

TEST(freq_estimator, large_queue_size) {
    for (size_t n = 0; n < FE_N_ITERATIONS; n++) {
        fe.update(FE_AIM * 2);
    }

    CHECK(fe.freq_coeff() > 1.0f);
}

TEST(freq_estimator, small_queue_size) {
    for (size_t n = 0; n < FE_N_ITERATIONS; n++) {
        fe.update(FE_AIM / 2);
    }

    CHECK(fe.freq_coeff() < 1.0f);
}

} // namespace test
} // namespace roc
