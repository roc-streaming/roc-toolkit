/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_HELPERS_MEDIAN_H_
#define ROC_AUDIO_TEST_HELPERS_MEDIAN_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace audio {
namespace test {

// 1D median filter.
// http://www.librow.com/articles/article-1
static void median_filter(const double* signal, double* result, size_t size) {
    enum { WindowSize = 9 };

    for (size_t i = WindowSize / 2; i < size - WindowSize / 2; i++) {
        // Fill window.
        double window[WindowSize];
        for (size_t j = 0; j < WindowSize; ++j) {
            window[j] = signal[i - WindowSize / 2 + j];
        }

        // Sort the first half of the window.
        for (size_t j = 0; j <= WindowSize / 2; ++j) {
            size_t min_index = j;
            for (size_t k = j + 1; k < WindowSize; ++k) {
                if (window[k] < window[min_index]) {
                    min_index = k;
                }
            }
            const double temp = window[j];
            window[j] = window[min_index];
            window[min_index] = temp;
        }

        // Store the median into the result.
        result[i - WindowSize / 2] = window[WindowSize / 2];
    }

    // Fill tail.
    for (size_t i = size - WindowSize; i < size; i++) {
        result[i] = result[size - WindowSize];
    }
}

} // namespace test
} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TEST_HELPERS_MEDIAN_H_
