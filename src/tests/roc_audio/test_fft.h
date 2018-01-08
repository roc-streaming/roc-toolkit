/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_FFT_H_
#define ROC_AUDIO_TEST_FFT_H_

namespace roc {
namespace audio {

void FFT(double* data, unsigned long nn);
void FreqSpectrum(double* data, size_t n);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TEST_FFT_H_
