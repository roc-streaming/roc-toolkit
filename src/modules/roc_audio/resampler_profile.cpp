/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_profile.h"

namespace roc {
namespace audio {

ResamplerConfig resampler_profile(ResamplerProfile profile) {
    ResamplerConfig config;

    switch (profile) {
    case ResamplerProfile_Low:
        config.window_interp = 64;
        config.window_size = 16;
        break;

    case ResamplerProfile_Medium:
        config.window_interp = 128;
        config.window_size = 32;
        break;

    case ResamplerProfile_High:
        config.window_interp = 512;
        config.window_size = 64;
        break;
    }

    return config;
}

} // namespace audio
} // namespace roc
