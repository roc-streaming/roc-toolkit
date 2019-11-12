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

#include <speex/speex_resampler.h>

#include "roc_audio/frame.h"
#include "roc_audio/ireader.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/resampler_profile.h"
#include "roc_audio/units.h"
#include "roc_core/array.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Resamples audio stream using speex resampler.
class SpeexResampler : public IResampler, public core::NonCopyable<> {
public:
    //! Initialize.
    //! @remarks
    //! quality is an integer in the range 0-10 inclusive,
    //! where 10 is the best quality and 0 is the worst quality
    SpeexResampler(core::IAllocator& allocator,
                   ResamplerProfile profile,
                   core::nanoseconds_t frame_length,
                   size_t sample_rate,
                   packet::channel_mask_t channels);

    ~SpeexResampler();

    bool valid() const;

    bool
    set_scaling(size_t input_sample_rate, size_t output_sample_rate, float multiplier);

    bool resample_buff(Frame& out);

    void renew_buffers(core::Slice<sample_t>& prev,
                       core::Slice<sample_t>& cur,
                       core::Slice<sample_t>& next);

private:
    bool refresh_state_();

    SpeexResamplerState* speex_state_;

    sample_t* prev_frame_;
    sample_t* curr_frame_;
    sample_t* next_frame_;

    core::Array<sample_t> mix_frame_;

    spx_uint32_t out_frame_pos_;
    spx_uint32_t in_offset_;

    const spx_uint32_t frame_size_;

    spx_uint32_t input_sample_rate_;
    spx_uint32_t output_sample_rate_;
    float sample_rate_multiplier_;

    int quality_;

    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_SPEEX_H_
