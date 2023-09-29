/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/target_speexdsp/roc_audio/resampler_speex.h
//! @brief Speex resampler.

#ifndef ROC_AUDIO_RESAMPLER_SPEEX_H_
#define ROC_AUDIO_RESAMPLER_SPEEX_H_

#include "roc_audio/frame.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/resampler_profile.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/array.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

#include <speex/speex_resampler.h>

namespace roc {
namespace audio {

//! Speex resampler.
//!
//! Resamples audio stream using SpeexDSP library.
//!
//! This backend is very fast even on weak CPUs, and provides good quality,
//! but it can't apply requested scaling very precisely.
class SpeexResampler : public IResampler, public core::NonCopyable<> {
public:
    //! Initialize.
    SpeexResampler(core::IArena& arena,
                   core::BufferFactory<sample_t>& buffer_factory,
                   ResamplerProfile profile,
                   const audio::SampleSpec& in_spec,
                   const audio::SampleSpec& out_spec);

    ~SpeexResampler();

    //! Check if object is successfully constructed.
    virtual bool is_valid() const;

    //! Set new resample factor.
    virtual bool set_scaling(size_t input_rate, size_t output_rate, float multiplier);

    //! Get buffer to be filled with input data.
    virtual const core::Slice<sample_t>& begin_push_input();

    //! Commit buffer with input data.
    virtual void end_push_input();

    //! Read samples from input frame and fill output frame.
    virtual size_t pop_output(Frame& out);

    //! How many samples were pushed but not processed yet.
    virtual float n_left_to_process() const;

private:
    void report_stats_();

    SpeexResamplerState* speex_state_;

    core::Slice<sample_t> in_frame_;
    const spx_uint32_t in_frame_size_;
    spx_uint32_t in_frame_pos_;

    const spx_uint32_t num_ch_;

    core::RateLimiter rate_limiter_;

    bool valid_;

    // Counts how many output samples to throw away in order to
    // compensate resampler's inner latency.
    size_t startup_delay_compensator_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_SPEEX_H_
