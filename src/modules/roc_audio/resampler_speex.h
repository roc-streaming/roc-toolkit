/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/resampler_speex.h
//! @brief Resampler Speex.

#ifndef ROC_AUDIO_RESAMPLER_SPEEX_H_
#define ROC_AUDIO_RESAMPLER_SPEEX_H_

#include "roc_audio/iresampler.h"
#include "roc_audio/frame.h"
#include "roc_audio/ireader.h"
#include "roc_audio/units.h"
#include "roc_audio/resampler_config.h"
#include "roc_core/array.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

class SpeexResampler : public IResampler{
public:
    SpeexResampler(core::IAllocator& allocator,
              const ResamplerConfig& config,
              packet::channel_mask_t channels,
              size_t frame_size);

    bool valid() const;

    bool set_scaling(float);

    bool resample_buff(Frame& out);

    void renew_buffers(core::Slice<sample_t>& prev,
                       core::Slice<sample_t>& cur,
                       core::Slice<sample_t>& next);

    ~SpeexResampler();

private:
   
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_SPEEX_H_
