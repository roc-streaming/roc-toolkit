/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/resampler_config.h
//! @brief Resampler.

#ifndef ROC_AUDIO_RESAMPLER_CONFIG_H_
#define ROC_AUDIO_RESAMPLER_CONFIG_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace audio {
//! Resampler parameters.
struct ResamplerConfig {
    //! Sinc table precision.
    //! @remarks
    //!  Affects sync table size.
    //!  Lower values give lower quality but rarer cache misses.
    size_t window_interp;

    //! Resampler internal window length.
    //! @remarks
    //!  Affects sync table size and number of CPU cycles.
    //!  Lower values give lower quality but higher speed and also rarer cache misses.
    size_t window_size;

    ResamplerConfig()
        : window_interp(128)
        , window_size(32) {
    }
};

//! Indexes the backend used for the resampling.
enum ResamplerBackend { ResamplerBackend_Builtin = 0 };

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_CONFIG_H_
