/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/units.h
//! @brief Various units used in audio processing.

#ifndef ROC_AUDIO_UNITS_H_
#define ROC_AUDIO_UNITS_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

//! Audio sample.
typedef float sample_t;

//! Maximum possible value of a sample.
extern const sample_t SampleMax;

//! Minimum possible value of a sample.
extern const sample_t SampleMin;

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_UNITS_H_
