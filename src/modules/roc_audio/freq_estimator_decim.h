/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/freq_estimator_decim.h
//! @brief Frequency estimator config.

#ifndef ROC_AUDIO_FREQ_ESTIMATOR_DECIM_H_
#define ROC_AUDIO_FREQ_ESTIMATOR_DECIM_H_

#include "roc_audio/units.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

// Frequency estimator decimator factor.
static const size_t fe_decim_factor = 10;

//! Length of decimation filter response length in frequency estimator.
//! @remarks
//!  Should be power of two.
static const size_t fe_decim_len = 256;

//! Bitmask for fe_decim_len.
static const uint32_t fe_decim_len_mask = fe_decim_len - 1;

//! Impulse response of decimation filter with factor of 10.
extern const sample_t fe_decim_h[fe_decim_len];

//! Filters gain, sum(fe_decim_h).
extern const sample_t fe_decim_h_gain;

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FREQ_ESTIMATOR_DECIM_H_
