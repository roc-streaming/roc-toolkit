/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/resampler_speex.h
//! @brief Resampler Speex.

#ifndef ROC_AUDIO_RESAMPLER_SPEEX_H_
#define ROC_AUDIO_RESAMPLER_SPEEX_H_

#include "roc_audio/frame.h"
#include "roc_audio/ireader.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/resampler_config.h"
#include "roc_audio/units.h"
#include "roc_core/array.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

#include <speex/speex_resampler.h>

namespace roc {
namespace audio {
    namespace {
        const uint32_t FRACT_BIT_COUNT = 20;
        typedef int32_t signed_fixedpoint_t;
        typedef uint32_t fixedpoint_t;
    };
    


class SpeexResampler : public IResampler {
public:
    SpeexResampler(core::IAllocator& allocator,
                   const ResamplerConfig& config,
                   packet::channel_mask_t channels,
                   size_t frame_size);

    bool valid() const;

    bool set_scaling(float input_sample_rate, float output_sample_rate, float multiplier);

    bool resample_buff(Frame& out);

    void renew_buffers(core::Slice<sample_t>& prev,
                       core::Slice<sample_t>& cur,
                       core::Slice<sample_t>& next);

    ~SpeexResampler();

private:
    core::IAllocator& allocator;
    const packet::channel_mask_t channel_mask_;
    const size_t channels_num_;

    SpeexResamplerState* speex_state;

    bool refresh_state();

    sample_t* prev_frame_;
    sample_t* curr_frame_;
    sample_t* next_frame_;

    size_t out_frame_pos_;

    float scaling_;

    const size_t frame_size_;
    const size_t frame_size_ch_;

    float input_sample_rate_;
    float output_sample_rate_;
    float sample_rate_multiplier_;

    int counter;
    bool valid_;


    bool check_config_() const;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_SPEEX_H_
