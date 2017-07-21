/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/freq_estimator.h"

namespace roc {
namespace audio {

namespace {

enum { Aim = 10000 };

const float Epsilon = 0.0001f;

} // namespace

TEST_GROUP(freq_estimator){};

TEST(freq_estimator, initial) {
    FreqEstimator fe(Aim);

    DOUBLES_EQUAL(1.0, fe.freq_coeff(), Epsilon);
}

TEST(freq_estimator, aim_queue_size) {
    FreqEstimator fe(Aim);

    for (size_t n = 0; n < 1000; n++) {
        fe.update(Aim);
    }

    DOUBLES_EQUAL(1.0, fe.freq_coeff(), Epsilon);
}

TEST(freq_estimator, large_queue_size) {
    FreqEstimator fe(Aim);

    do {
        fe.update(Aim * 2);
    } while (fe.freq_coeff() < 1.01f);
}

TEST(freq_estimator, small_queue_size) {
    FreqEstimator fe(Aim);

    do {
        fe.update(Aim / 2);
    } while (fe.freq_coeff() > 0.99f);
}

} // namespace audio
} // namespace roc
