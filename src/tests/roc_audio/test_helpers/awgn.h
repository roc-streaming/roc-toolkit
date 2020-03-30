/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_HELPERS_AWGN_H_
#define ROC_AUDIO_TEST_HELPERS_AWGN_H_

#include "roc_core/fast_random.h"

namespace roc {
namespace audio {
namespace test {

// Generates additive white Gaussian Noise samples with zero mean
// and a standard deviation of 1.
// https://www.embeddedrelated.com/showcode/311.php
inline double generate_awgn() {
    enum { RandMax = 2000000000 };

    const double r1 = core::fast_random(0, RandMax) / (double)RandMax;
    const double r2 = core::fast_random(10, RandMax) / (double)RandMax;

    return sqrt(-2.0 * log(r2)) * cos(2.0 * M_PI * r1);
}

} // namespace test
} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TEST_HELPERS_AWGN_H_
