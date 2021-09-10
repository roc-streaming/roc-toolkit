/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_backend.h"

namespace roc {
namespace audio {

const char* resampler_backend_to_str(ResamplerBackend backend) {
    switch (backend) {
    case ResamplerBackend_Builtin:
        return "builtin";

    case ResamplerBackend_Speex:
        return "speex";

    case ResamplerBackend_Default:
        break;
    }

    return "default";
}

} // namespace audio
} // namespace roc
