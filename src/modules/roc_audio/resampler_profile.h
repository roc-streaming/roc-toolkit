/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/resampler_profile.h
//! @brief Resampler profile.

#ifndef ROC_AUDIO_RESAMPLER_PROFILE_H_
#define ROC_AUDIO_RESAMPLER_PROFILE_H_

namespace roc {
namespace audio {

//! Resampler parameters presets.
enum ResamplerProfile {
    //! Low quality, fast speed.
    ResamplerProfile_Low,

    //! Medium quality, medium speed.
    ResamplerProfile_Medium,

    //! Hight quality, low speed.
    ResamplerProfile_High
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_PROFILE_H_
