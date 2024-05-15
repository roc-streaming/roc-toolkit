/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/resampler_map.h
//! @brief Resampler map.

#ifndef ROC_AUDIO_RESAMPLER_MAP_H_
#define ROC_AUDIO_RESAMPLER_MAP_H_

#include "roc_audio/frame_factory.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/resampler_config.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/singleton.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Factory class for IResampler objects, according to the ResamplerBackend input
class ResamplerMap : public core::NonCopyable<> {
public:
    //! Get instance.
    static ResamplerMap& instance() {
        return core::Singleton<ResamplerMap>::instance();
    }

    //! Get number of backends.
    size_t num_backends() const;

    //! Get backend ID by number.
    ResamplerBackend nth_backend(size_t n) const;

    //! Check if given backend is supported.
    bool is_supported(ResamplerBackend backend_id) const;

    //! Instantiate IResampler for given backend ID.
    core::SharedPtr<IResampler> new_resampler(core::IArena& arena,
                                              FrameFactory& frame_factory,
                                              const ResamplerConfig& config,
                                              const audio::SampleSpec& in_spec,
                                              const audio::SampleSpec& out_spec);

private:
    friend class core::Singleton<ResamplerMap>;

    enum { MaxBackends = 4 };

    struct Backend {
        Backend()
            : id()
            , ctor(NULL) {
        }

        ResamplerBackend id;
        core::SharedPtr<IResampler> (*ctor)(core::IArena& arena,
                                            FrameFactory& frame_factory,
                                            ResamplerProfile profile,
                                            const audio::SampleSpec& in_spec,
                                            const audio::SampleSpec& out_spec);
    };

    ResamplerMap();

    void add_backend_(const Backend& backend);
    const Backend* find_backend_(ResamplerBackend) const;

    Backend backends_[MaxBackends];
    size_t n_backends_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_MAP_H_
