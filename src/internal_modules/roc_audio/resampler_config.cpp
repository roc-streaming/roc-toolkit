/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_config.h"
#include "roc_audio/resampler_map.h"

namespace roc {
namespace audio {

void ResamplerConfig::deduce_defaults(LatencyTunerBackend latency_backend,
                                      LatencyTunerProfile latency_tuner) {
    if (backend == ResamplerBackend_Default) {
        // If responsive profile is set, use builtin backend instead of speex,
        // since it has higher scaling precision.
        const bool need_builtin_backend = latency_tuner == LatencyTunerProfile_Responsive;

        // If speex backend is not available, fallback to builtin backend.
        const bool force_builtin_backend =
            !ResamplerMap::instance().is_supported(ResamplerBackend_Speex);

        if (need_builtin_backend || force_builtin_backend) {
            backend = ResamplerBackend_Builtin;
        } else {
            backend = ResamplerBackend_Speex;
        }
    }
}

const char* resampler_backend_to_str(ResamplerBackend backend) {
    switch (backend) {
    case ResamplerBackend_Builtin:
        return "builtin";

    case ResamplerBackend_Speex:
        return "speex";

    case ResamplerBackend_SpeexDec:
        return "speexdec";

    case ResamplerBackend_Default:
        return "default";
    }

    return "invalid";
}

const char* resampler_profile_to_str(ResamplerProfile profile) {
    switch (profile) {
    case ResamplerProfile_Low:
        return "low";

    case ResamplerProfile_Medium:
        return "medium";

    case ResamplerProfile_High:
        return "high";
    }

    return "invalid";
}

} // namespace audio
} // namespace roc
