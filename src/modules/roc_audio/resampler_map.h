/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/resampler_map.h
//! @brief Resampler map.

#ifndef ROC_AUDIO_RESAMPLER_MAP_H_
#define ROC_AUDIO_RESAMPLER_MAP_H_

#include "roc_audio/iresampler.h"
#include "roc_audio/resampler_backend.h"
#include "roc_audio/resampler_profile.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
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

    //! Method to instantiate and return a pointer to a IResampler object
    IResampler* new_resampler(ResamplerBackend resampler_backend,
                              core::IAllocator& allocator,
                              ResamplerProfile profile,
                              core::nanoseconds_t frame_length,
                              size_t sample_rate,
                              packet::channel_mask_t channels);

private:
    friend class core::Singleton<ResamplerMap>;

    enum { MaxBackends = 2 };

    struct Backend {
        Backend()
            : id()
            , ctor(NULL) {
        }

        ResamplerBackend id;
        IResampler* (*ctor)(core::IAllocator& allocator,
                            ResamplerProfile profile,
                            core::nanoseconds_t frame_length,
                            size_t sample_rate,
                            packet::channel_mask_t channels);
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
