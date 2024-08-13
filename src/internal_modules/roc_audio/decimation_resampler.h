/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/decimation_resampler.h
//! @brief Decimating resampler.

#ifndef ROC_AUDIO_DECIMATION_RESAMPLER_H_
#define ROC_AUDIO_DECIMATION_RESAMPLER_H_

#include "roc_audio/frame.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

//! Decimating resampler.
//!
//! Acts as decorator for another resampler instance.
//!
//! Performs resampling in two stages:
//!  - first, uses underlying resampler to apply constant part of scaling factor based
//!    on input and output rates; if these rates are equal, first stage is skipped
//!  - then, uses decimation or duplication to apply dynamic part of scaling
//!    factor, a.k.a. multiplier, by dropping or duplicating samples
//!
//! When input and output rates are the same, this backend implements fastest possible
//! resampling algorithm working almost at the speed of memcpy().
//!
//! Although decimation usually degrades quality a lot, it's not so dramatic in this
//! specific case because we use it only for dynamic part of scaling factor, which in
//! practice is very close to 1.0, and typically we remove or insert up to 20 samples
//! per second or so on 48kHz, which corresponds to ~ 0.4ms/second.
//!
//! When input and output rates are different, this backend uses another, underlying
//! resampler, but only for converting between input and output rates. It still uses
//! decimation or duplication for applying dynamic part of scaling factor.
class DecimationResampler : public IResampler, public core::NonCopyable<> {
public:
    //! Initialize.
    DecimationResampler(const core::SharedPtr<IResampler>& inner_resampler,
                        const SampleSpec& in_spec,
                        const SampleSpec& out_spec,
                        FrameFactory& frame_factory,
                        core::IArena& arena);

    ~DecimationResampler();

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

    const core::SharedPtr<IResampler> inner_resampler_;
    bool use_inner_resampler_;

    SampleSpec input_spec_;
    SampleSpec output_spec_;
    float multiplier_;

    const size_t num_ch_;

    core::Slice<sample_t> in_buf_;
    size_t in_size_;
    size_t in_pos_;

    float out_acc_;

    core::Slice<sample_t> last_buf_;

    size_t total_count_;
    size_t decim_count_;
    core::RateLimiter report_limiter_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_DECIMATION_RESAMPLER_H_
