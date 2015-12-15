/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/freq_estimator_decim10_len.h
//! @brief Frequency estimator config.

#ifndef ROC_AUDIO_FREQ_ESTIMATOR_DECIM10_LEN_H_
#define ROC_AUDIO_FREQ_ESTIMATOR_DECIM10_LEN_H_

//! Length of decimation filter response length in Frequency Estimator.
#define FREQ_EST_DECIM_10_LEN 256

#if (FREQ_EST_DECIM_10_LEN % 2) > 0
#error FREQ_EST_DECIM_10_LEN must be power of two
#endif

//! Bitmask for FREQ_EST_DECIM_10_LEN.
#define FREQ_EST_DECIM_10_LEN_MASK (uint32_t)(FREQ_EST_DECIM_10_LEN - 1)

#endif // ROC_AUDIO_FREQ_ESTIMATOR_DECIM10_LEN_H_
