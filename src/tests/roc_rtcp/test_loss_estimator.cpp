/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_rtcp/loss_estimator.h"

namespace roc {
namespace rtcp {

namespace {

const double Epsilon = 1e-8;

} // namespace

TEST_GROUP(loss_estimator) {};

TEST(loss_estimator, regular) {
    LossEstimator le;

    // 0, 0
    DOUBLES_EQUAL(0.0, le.update(0, 0), Epsilon);
    // +10, +0
    DOUBLES_EQUAL(0.0, le.update(10, 0), Epsilon);
    // +10, +5
    DOUBLES_EQUAL(0.5, le.update(20, 5), Epsilon);
    // +10, +1
    DOUBLES_EQUAL(0.1, le.update(30, 6), Epsilon);
    // +10, +0
    DOUBLES_EQUAL(0.0, le.update(40, 6), Epsilon);
    // +10, -1
    DOUBLES_EQUAL(0.0, le.update(50, 5), Epsilon);
    // +10, -10
    DOUBLES_EQUAL(0.0, le.update(60, -5), Epsilon);
    // +10, +10
    DOUBLES_EQUAL(1.0, le.update(70, 5), Epsilon);
    // +10, +2
    DOUBLES_EQUAL(0.2, le.update(80, 7), Epsilon);
}

TEST(loss_estimator, jump_nackwards) {
    LossEstimator le;

    // +40, +4
    DOUBLES_EQUAL(0.1, le.update(40, 4), Epsilon);
    // -30, +2
    DOUBLES_EQUAL(0.0, le.update(10, 6), Epsilon);
    // +10, +2
    DOUBLES_EQUAL(0.2, le.update(20, 8), Epsilon);
}

} // namespace rtcp
} // namespace roc
