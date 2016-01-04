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

enum { Aim = ROC_CONFIG_DEFAULT_SESSION_LATENCY * 2, Iterations = 1000 };

const double Epsilon = 1e-6;

} // namespace

TEST_GROUP(freq_estimator){};

TEST(freq_estimator, initial) {
    FreqEstimator fe(Aim);

    DOUBLES_EQUAL(1.0, fe.freq_coeff(), Epsilon);
}

TEST(freq_estimator, aim_queue_size) {
    FreqEstimator fe(Aim);

    for (size_t n = 0; n < Iterations; n++) {
        fe.update(Aim);
    }

    DOUBLES_EQUAL(1.0, fe.freq_coeff(), Epsilon);
}

TEST(freq_estimator, large_queue_size) {
    FreqEstimator fe(Aim);

    for (size_t n = 0; n < Iterations; n++) {
        fe.update(Aim * 2);
    }

    CHECK(fe.freq_coeff() > 1.0f);
}

TEST(freq_estimator, small_queue_size) {
    FreqEstimator fe(Aim);

    for (size_t n = 0; n < Iterations; n++) {
        fe.update(Aim / 2);
    }

    CHECK(fe.freq_coeff() < 1.0f);
}

} // namespace test
} // namespace roc
