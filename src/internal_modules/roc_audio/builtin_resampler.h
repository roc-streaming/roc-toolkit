/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/builtin_resampler.h
//! @brief Built-in resampler.

#ifndef ROC_AUDIO_BUILTIN_RESAMPLER_H_
#define ROC_AUDIO_BUILTIN_RESAMPLER_H_

#include "roc_audio/frame.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/resampler_config.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/array.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Built-in resampler.
//!
//! Resamples audio stream with non-integer dynamically changing factor.
//! Implements bandlimited interpolation from this paper:
//!   https://ccrma.stanford.edu/~jos/resample/resample.pdf
//!
//! This backend is quite CPU-hungry, but it maintains requested scaling
//! factor with very high precision.
class BuiltinResampler : public IResampler, public core::NonCopyable<> {
public:
    //! Initialize.
    BuiltinResampler(const ResamplerConfig& config,
                     const SampleSpec& in_spec,
                     const SampleSpec& out_spec,
                     FrameFactory& frame_factory,
                     core::IArena& arena);

    ~BuiltinResampler();

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Set new resample factor.
    //! @remarks
    //!  Resampling algorithm needs some window of input samples. The length of the window
    //!  (length of sinc impulse response) is a compromise between SNR and speed. It
    //!  depends on current resampling factor. So we choose length of input buffers to let
    //!  it handle maximum length of input. If new scaling factor breaks equation this
    //!  function returns false.
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
    typedef uint32_t fixedpoint_t;
    typedef uint64_t long_fixedpoint_t;
    typedef int32_t signed_fixedpoint_t;
    typedef int64_t signed_long_fixedpoint_t;

    inline size_t channelize_index(const size_t i, const size_t ch_offset) const {
        return i * in_spec_.num_channels() + ch_offset;
    }

    bool alloc_frames_(FrameFactory& frame_factory);

    bool check_config_() const;

    bool fill_sinc_();
    sample_t sinc_(fixedpoint_t x, float fract_x);

    // Computes single sample of the particular audio channel.
    // channel_offset a serial number of the channel
    // (e.g. left -- 0, right -- 1, etc.).
    sample_t resample_(size_t channel_offset);

    const SampleSpec in_spec_;
    const SampleSpec out_spec_;

    core::Slice<sample_t> frames_[3];
    size_t n_ready_frames_;

    const sample_t* prev_frame_;
    const sample_t* curr_frame_;
    const sample_t* next_frame_;

    float scaling_;

    const size_t window_size_;
    const fixedpoint_t qt_half_sinc_window_size_;

    const size_t window_interp_;
    const size_t window_interp_bits_;

    const size_t frame_size_ch_;
    const size_t frame_size_;

    core::Array<sample_t> sinc_table_;
    const sample_t* sinc_table_ptr_;

    // half window len in Q8.24 in terms of input signal
    fixedpoint_t qt_half_window_size_;
    const fixedpoint_t qt_epsilon_;

    const fixedpoint_t qt_frame_size_;

    // time position of output sample in terms of input samples indexes
    // for example 0 -- time position of first sample in curr_frame_
    fixedpoint_t qt_sample_;

    // time distance between two output samples, equals to resampling factor
    fixedpoint_t qt_dt_;

    // the step with which we iterate over the sinc_table_
    fixedpoint_t qt_sinc_step_;

    const sample_t cutoff_freq_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_BUILTIN_RESAMPLER_H_
