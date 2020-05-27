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
    //! Default backend.
    ResamplerBackend_Default,

    //! Roc built-in resampler.
    ResamplerBackend_Builtin,

    //! SpeexDSP resampler.
    ResamplerBackend_Speex
};

//! Get string name of resampler backend.
const char* resampler_backend_to_str(ResamplerBackend);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_BACKEND_H_
