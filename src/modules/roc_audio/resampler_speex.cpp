/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_speex.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

#include <speex/speex_resampler.h>

namespace roc {
namespace audio {


SpeexResampler::SpeexResampler(core::IAllocator& allocator,
                     const ResamplerConfig& config,
                     packet::channel_mask_t channels,
                     size_t frame_size){
   
}

SpeexResampler::~SpeexResampler(){}

bool SpeexResampler::valid() const {
    return true;
}

bool SpeexResampler::set_scaling(float new_scaling) {
   return true;
}

bool SpeexResampler::resample_buff(Frame& out) {

    return true;
}



void SpeexResampler::renew_buffers(core::Slice<sample_t>& prev,
                              core::Slice<sample_t>& cur,
                              core::Slice<sample_t>& next) {
}



} // namespace audio
} // namespace roc
