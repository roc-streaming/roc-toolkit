/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_builtin.h"
#include "roc_audio/resampler_speex.h"

namespace roc {
namespace audio {

ResamplerMap::ResamplerMap() {
}

size_t ResamplerMap::num_backends() const {
    return NumResamplerBackends;
}

ResamplerBackend ResamplerMap::nth_backend(size_t n) const {
    roc_panic_if_not(n < (size_t)NumResamplerBackends);

    return (ResamplerBackend)n;
}

IResampler* ResamplerMap::new_resampler(ResamplerBackend resampler_backend,
                                        core::IAllocator& allocator,
                                        ResamplerProfile profile,
                                        core::nanoseconds_t frame_length,
                                        size_t sample_rate,
                                        packet::channel_mask_t channels) {
    core::ScopedPtr<IResampler> resampler;

    switch ((int)resampler_backend) {
    case ResamplerBackend_Builtin:
        resampler.reset(new (allocator) BuiltinResampler(allocator, profile, frame_length,
                                                         sample_rate, channels),
                        allocator);
        break;

    case ResamplerBackend_Speex:
        resampler.reset(new (allocator) SpeexResampler(allocator, profile, frame_length,
                                                       sample_rate, channels),
                        allocator);
        break;

    default:
        roc_panic("resampler map: invalid resampler backend %d", resampler_backend);
    }

    if (!resampler || !resampler->valid()) {
        return NULL;
    }

    return resampler.release();
}

} // namespace audio
} // namespace roc
