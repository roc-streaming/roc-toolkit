/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_HELPERS_FFT_H_
#define ROC_AUDIO_TEST_HELPERS_FFT_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace audio {
namespace test {

void fft(double* data, unsigned long nn);
void freq_spectrum(double* data, size_t n);

} // namespace test
} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TEST_HELPERS_FFT_H_
