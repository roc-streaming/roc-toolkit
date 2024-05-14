/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_map.h"
#include "roc_audio/builtin_resampler.h"
#include "roc_audio/decimation_resampler.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/scoped_ptr.h"

#ifdef ROC_TARGET_SPEEXDSP
#include "roc_audio/speex_resampler.h"
#endif // ROC_TARGET_SPEEXDSP

namespace roc {
namespace audio {

namespace {

template <class T>
core::SharedPtr<IResampler> resampler_ctor(core::IArena& arena,
                                           core::BufferFactory& buffer_factory,
                                           ResamplerProfile profile,
                                           const audio::SampleSpec& in_spec,
                                           const audio::SampleSpec& out_spec) {
    return new (arena) T(arena, buffer_factory, profile, in_spec, out_spec);
}

template <class T>
core::SharedPtr<IResampler> resampler_dec_ctor(core::IArena& arena,
                                               core::BufferFactory& buffer_factory,
                                               ResamplerProfile profile,
                                               const audio::SampleSpec& in_spec,
                                               const audio::SampleSpec& out_spec) {
    core::SharedPtr<IResampler> inner_resampler =
        new (arena) T(arena, buffer_factory, profile, in_spec, out_spec);

    return new (arena)
        DecimationResampler(inner_resampler, arena, buffer_factory, in_spec, out_spec);
}

} // namespace

ResamplerMap::ResamplerMap()
    : n_backends_(0) {
#ifdef ROC_TARGET_SPEEXDSP
    {
        Backend back;
        back.id = ResamplerBackend_Speex;
        back.ctor = &resampler_ctor<SpeexResampler>;
        add_backend_(back);
    }
    {
        Backend back;
        back.id = ResamplerBackend_SpeexDec;
        back.ctor = &resampler_dec_ctor<SpeexResampler>;
        add_backend_(back);
    }
#endif // ROC_TARGET_SPEEXDSP
    {
        Backend back;
        back.id = ResamplerBackend_Builtin;
        back.ctor = &resampler_ctor<BuiltinResampler>;
        add_backend_(back);
    }
}

size_t ResamplerMap::num_backends() const {
    return n_backends_;
}

ResamplerBackend ResamplerMap::nth_backend(size_t n) const {
    roc_panic_if_not(n < n_backends_);
    return backends_[n].id;
}

bool ResamplerMap::is_supported(ResamplerBackend backend_id) const {
    return find_backend_(backend_id) != NULL;
}

core::SharedPtr<IResampler>
ResamplerMap::new_resampler(core::IArena& arena,
                            core::BufferFactory& buffer_factory,
                            const ResamplerConfig& config,
                            const audio::SampleSpec& in_spec,
                            const audio::SampleSpec& out_spec) {
    const Backend* backend = find_backend_(config.backend);
    if (!backend) {
        roc_log(LogError, "resampler map: unsupported resampler backend: [%d] %s",
                config.backend, resampler_backend_to_str(config.backend));
        return NULL;
    }

    core::SharedPtr<IResampler> resampler =
        backend->ctor(arena, buffer_factory, config.profile, in_spec, out_spec);

    if (!resampler || !resampler->is_valid()) {
        return NULL;
    }

    return resampler;
}

void ResamplerMap::add_backend_(const Backend& backend) {
    roc_panic_if(n_backends_ == MaxBackends);
    backends_[n_backends_++] = backend;
}

const ResamplerMap::Backend*
ResamplerMap::find_backend_(ResamplerBackend backend_id) const {
    for (size_t n = 0; n < n_backends_; n++) {
        if (backends_[n].id == backend_id) {
            return &backends_[n];
        }
    }
    return NULL;
}

} // namespace audio
} // namespace roc
