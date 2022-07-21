/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/sample.h
//! @brief Audio sample.

#ifndef ROC_AUDIO_SAMPLE_H_
#define ROC_AUDIO_SAMPLE_H_

#include "roc_audio/pcm_format.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

//! Audio sample.
typedef float sample_t;

//! Sample format description.
extern const PcmFormat SampleFormat;

//! Minimum possible value of a sample.
extern const sample_t SampleMin;

//! Maximum possible value of a sample.
extern const sample_t SampleMax;

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SAMPLE_H_
