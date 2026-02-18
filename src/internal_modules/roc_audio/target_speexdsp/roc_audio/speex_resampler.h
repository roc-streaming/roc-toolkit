/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/target_speexdsp/roc_audio/speex_resampler.h
//! @brief Speex resampler.

#ifndef ROC_AUDIO_SPEEX_RESAMPLER_H_
#define ROC_AUDIO_SPEEX_RESAMPLER_H_

#include "roc_audio/frame.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/resampler_config.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/array.h"
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
    SpeexResampler(const ResamplerConfig& config,
                   const SampleSpec& in_spec,
                   const SampleSpec& out_spec,
                   FrameFactory& frame_factory,
                   core::IArena& arena);

    ~SpeexResampler();

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Set new resample factor.
    virtual bool set_scaling(size_t input_rate, size_t output_rate, float multiplier);

    //! Get buffer to be filled with input data.
    virtual const core::Slice<sample_t>& begin_push_input();

    //! Commit buffer with input data.
    virtual void end_push_input();

    //! Read samples from input frame and fill output frame.
    virtual size_t pop_output(sample_t* out_data, size_t out_size);

    //! How many samples were pushed but not processed yet.
    virtual float n_left_to_process() const;

private:
    void report_stats_();

    SpeexResamplerState* speex_state_;

    // Channel count.
    const spx_uint32_t num_ch_;

    // Frame with input samples.
    core::Slice<sample_t> in_frame_;
    spx_uint32_t in_frame_size_;
    spx_uint32_t in_frame_pos_;

    // Counts how many output samples to throw away in order to
    // compensate resampler's inner latency.
    size_t initial_out_countdown_;

    // Stores initial latency in order to track its further changes.
    size_t initial_in_latency_;

    // Stores how much speex resampler latency changed from the start, in order to
    // reflect it in n_left_to_process() for better precision in capture timestamp
    // calculations.
    ssize_t in_latency_diff_;

    core::RateLimiter report_limiter_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SPEEX_RESAMPLER_H_
