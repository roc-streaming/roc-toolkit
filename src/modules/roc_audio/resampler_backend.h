/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/resampler_backend.h
//! @brief Resampler backend.

#ifndef ROC_AUDIO_RESAMPLER_BACKEND_H_
#define ROC_AUDIO_RESAMPLER_BACKEND_H_

namespace roc {
namespace audio {

//! Resampler backends.
enum ResamplerBackend {
    //! Roc built-in resampler.
    ResamplerBackend_Builtin = 0,

    //! SpeexDSP resampler.
    ResamplerBackend_Speex = 1,

    //! Number of backends.
    NumResamplerBackends
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_BACKEND_H_
