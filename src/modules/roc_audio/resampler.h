/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/resampler.h
//! @brief Resampler.

#ifndef ROC_AUDIO_RESAMPLER_H_
#define ROC_AUDIO_RESAMPLER_H_

#include "roc_audio/frame.h"
#include "roc_audio/ireader.h"
#include "roc_audio/units.h"
#include "roc_core/array.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Resampler parameters.
struct ResamplerConfig {
    //! Resampler internal window length.
    size_t window_size;

    //! Resampler internal frame size.
    size_t frame_size;

    ResamplerConfig()
        : window_size(64)
        , frame_size(256) {
    }
};

//! Resamples audio stream with non-integer dynamically changing factor.
//! @remarks
//!  Typicaly being used with factor close to 1 ( 0.9 < factor < 1.1 ).
class Resampler : public IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader specifies input audio stream used in read()
    //!  - @p buffer_pool is used to allocate temporary buffers
    //!  - @p frame_size is number of samples per resampler frame per audio channel
    //!  - @p channels is the bitmask of audio channels
    Resampler(IReader& reader,
              core::BufferPool<sample_t>& buffer_pool,
              core::IAllocator& allocator,
              const ResamplerConfig& config,
              packet::channel_mask_t channels);

    //! Read audio frame.
    //! @remarks
    //!  Calculates everything during this call so it may take time.
    virtual void read(Frame&);

    //! Set new resample factor.
    //! @remarks
    //!  Resampling algorithm needs some window of input samples. The length of the window
    //!  (length of sinc impulse response) is a compromise between SNR and speed. It
    //!  depends on current resampling factor. So we choose length of input buffers to let
    //!  it handle maximum length of input. If new scaling factor breaks equation this
    //!  function returns false.
    bool set_scaling(float);

private:
    typedef uint32_t fixedpoint_t;
    typedef uint64_t long_fixedpoint_t;
    typedef int32_t signed_fixedpoint_t;
    typedef int64_t signed_long_fixedpoint_t;

    const packet::channel_mask_t channel_mask_;
    const size_t channels_num_;

    //! Computes single sample of the particular audio channel.
    //!
    //! @param channel_offset a serial number of the channel
    //!  (e.g. left -- 0, right -- 1, etc.).
    sample_t resample_(const size_t channel_offset);

    inline size_t channelize_index(const size_t i, const size_t ch_offset) const {
        return i * channels_num_ + ch_offset;
    }

    void init_window_(core::BufferPool<sample_t>&);
    void renew_window_();
    void fill_sinc();
    inline sample_t sinc_(const fixedpoint_t x, const float fract_x);

    // Input stream.
    IReader& reader_;

    // Input stream window.
    Frame window_[3];

    // Pointers to 3 frames of stream window.
    sample_t* prev_frame_;
    sample_t* curr_frame_;
    sample_t* next_frame_;

    //! Resampling factor.
    //!
    //! s_out_step / s_in_step = Fs_from / Fs_to.
    float scaling_;

    // Frame size.
    const size_t frame_size_;
    // (frame_size_ / st_Nwindow) is maximum allowed scaling ratio.
    const size_t channel_len_;

    const size_t window_len_;
    fixedpoint_t qt_half_sinc_window_len_;
    const size_t window_interp_;
    const size_t window_interp_bits_; //!< The number of bits in window_interp_.
    core::Array<sample_t> sinc_table_;

    // half window len in Q8.24 in terms of input signal.
    fixedpoint_t qt_half_window_len_;
    const fixedpoint_t qt_epsilon_;
    const fixedpoint_t default_sample_;

    // Frame size in Q8.24.
    const fixedpoint_t qt_frame_size_;

    // Time position of output sample in terms of input samples indexes.
    // For example 0 -- time position of first sample in curr_frame_.
    fixedpoint_t qt_sample_;

    // Time distance between two output samples, equals to resampling factor.
    fixedpoint_t qt_dt_;

    // The step with which we iterate over the sinc_table_.
    fixedpoint_t qt_sinc_step_;

    const sample_t cutoff_freq_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_H_
