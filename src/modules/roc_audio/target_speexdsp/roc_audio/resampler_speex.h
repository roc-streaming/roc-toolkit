/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/target_speexdsp/roc_audio/resampler_speex.h
//! @brief Resampler Speex.

#ifndef ROC_AUDIO_RESAMPLER_SPEEX_H_
#define ROC_AUDIO_RESAMPLER_SPEEX_H_

#include "roc_audio/frame.h"
#include "roc_audio/ireader.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/resampler_profile.h"
#include "roc_audio/units.h"
#include "roc_core/array.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

#include <speex/speex_resampler.h>

namespace roc {
namespace audio {

//! Resamples audio stream using speex resampler.
class SpeexResampler : public IResampler, public core::NonCopyable<> {
public:
    //! Initialize.
    SpeexResampler(core::IAllocator& allocator,
                   core::BufferPool<sample_t>& buffer_pool,
                   ResamplerProfile profile,
                   core::nanoseconds_t frame_length,
                   size_t sample_rate,
                   packet::channel_mask_t channels);

    ~SpeexResampler();

    //! Check if object is successfully constructed.
    virtual bool valid() const;

    //! Set new resample factor.
    virtual bool set_scaling(size_t input_rate, size_t output_rate, float multiplier);

    //! Get buffer to be filled with input data.
    virtual const core::Slice<sample_t>& begin_push_input();

    //! Commit buffer with input data.
    virtual void end_push_input();

    //! Read samples from input frame and fill output frame.
    virtual size_t pop_output(Frame& out);

private:
    void report_stats_();

    SpeexResamplerState* speex_state_;

    core::Slice<sample_t> in_frame_;
    const spx_uint32_t in_frame_size_;
    spx_uint32_t in_frame_pos_;

    const spx_uint32_t num_ch_;

    core::RateLimiter rate_limiter_;

    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_SPEEX_H_
