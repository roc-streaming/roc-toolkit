/*
 * Copyright (c) 2015 Roc authors
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
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Resampler parameters.
struct ResamplerConfig {
    //! Sinc table precision.
    //! @remarks
    //!  Affects sync table size.
    //!  Lower values give lower quality but rarer cache misses.
    size_t window_interp;

    //! Resampler internal window length.
    //! @remarks
    //!  Affects sync table size and number of CPU cycles.
    //!  Lower values give lower quality but higher speed and also rarer cache misses.
    size_t window_size;

    //! Resampler internal frame size.
    //! @remarks
    //!  Defines how much samples resampler requests from the underlying reader.
    //!  Lower values give lower latency and lower memory usage.
    size_t frame_size;

    ResamplerConfig()
        : window_interp(128)
        , window_size(32)
        , frame_size(256) {
    }
};

//! Resamples audio stream with non-integer dynamically changing factor.
class Resampler : public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p frame_size is number of samples per resampler frame per audio channel
    //!  - @p channels is the bitmask of audio channels
    Resampler(core::IAllocator& allocator,
              const ResamplerConfig& config,
              packet::channel_mask_t channels);

    //! Check if object is successfully constructed.
    bool valid() const;

    //! Set new resample factor.
    //! @remarks
    //!  Resampling algorithm needs some window of input samples. The length of the window
    //!  (length of sinc impulse response) is a compromise between SNR and speed. It
    //!  depends on current resampling factor. So we choose length of input buffers to let
    //!  it handle maximum length of input. If new scaling factor breaks equation this
    //!  function returns false.
    bool set_scaling(float);

    //! Resamples the whole output frame.
    bool resample_buff(Frame& out);

    //! Push new buffer on the front of the internal FIFO, which comprisesthree window_.
    void renew_buffers(core::Slice<sample_t>& prev,
                       core::Slice<sample_t>& cur,
                       core::Slice<sample_t>& next);

private:
    typedef uint32_t fixedpoint_t;
    typedef uint64_t long_fixedpoint_t;
    typedef int32_t signed_fixedpoint_t;
    typedef int64_t signed_long_fixedpoint_t;

    const packet::channel_mask_t channel_mask_;
    const size_t channels_num_;

    inline size_t channelize_index(const size_t i, const size_t ch_offset) const {
        return i * channels_num_ + ch_offset;
    }

    //! Computes single sample of the particular audio channel.
    //!
    //! @param channel_offset a serial number of the channel
    //!  (e.g. left -- 0, right -- 1, etc.).
    sample_t resample_(size_t channel_offset);

    bool check_config_() const;

    bool fill_sinc_();
    sample_t sinc_(fixedpoint_t x, float fract_x);

    // Pointers to 3 frames of stream window.
    sample_t* prev_frame_;
    sample_t* curr_frame_;
    sample_t* next_frame_;

    size_t out_frame_i_;

    //! Resampling factor.
    //! s_out_step / s_in_step = Fs_from / Fs_to.
    float scaling_;

    // Window size.
    // (window_size_ / st_Nwindow) is maximum allowed scaling ratio.
    const size_t window_size_;
    const size_t channel_len_;

    const size_t window_len_;
    const fixedpoint_t qt_half_sinc_window_len_;
    const size_t window_interp_;
    const size_t window_interp_bits_; //!< The number of bits in window_interp_.

    core::Array<sample_t> sinc_table_;
    const sample_t* sinc_table_ptr_;

    // half window len in Q8.24 in terms of input signal.
    fixedpoint_t qt_half_window_len_;
    const fixedpoint_t qt_epsilon_;

    // Frame size in Q8.24.
    const fixedpoint_t qt_window_size_;

    // Time position of output sample in terms of input samples indexes.
    // For example 0 -- time position of first sample in curr_frame_.
    fixedpoint_t qt_sample_;

    // Time distance between two output samples, equals to resampling factor.
    fixedpoint_t qt_dt_;

    // The step with which we iterate over the sinc_table_.
    fixedpoint_t qt_sinc_step_;

    const sample_t cutoff_freq_;

    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_H_
