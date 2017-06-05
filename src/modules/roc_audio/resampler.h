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

#include "roc_core/circular_buffer.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

#include "roc_audio/istream_reader.h"
#include "roc_audio/sample_buffer.h"

namespace roc {
namespace audio {

//! Resamples audio stream with non-integer dynamically changing factor.
//! @remarks
//!  Typicaly being used with factor close to 1 ( 0.9 < factor < 1.1 ).
class Resampler : public IStreamReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader specifies input audio stream used in read();
    //!  - @p composer is used to construct temporary buffers;
    //!  - @p frame_size is number of samples per resampler frame.
    explicit Resampler(IStreamReader& reader,
                       ISampleBufferComposer& composer = default_buffer_composer(),
                       size_t frame_size = ROC_CONFIG_DEFAULT_RESAMPLER_FRAME_SAMPLES);

    //! Fills buffer of samples with new sampling frequency.
    //! @remarks
    //!  Calculates everything during this call so it may take time.
    virtual void read(const ISampleBufferSlice&);

    //! Set new resample factor.
    //!
    //! Resampling algorithm needs some window of input samples. The length of the window
    //! (length of sinc impulse response) is a compromise between SNR and speed. It
    //! depends on current resampling factor. So we choose length of input buffers to let
    //! it handle maximum length of input. If new scaling factor breaks equation this
    //! function returns false.
    bool set_scaling(float);

private:
    typedef uint32_t fixedpoint_t;

    typedef packet::sample_t sample_t;

    sample_t resample_();

    void init_window_(ISampleBufferComposer&);
    void renew_window_();

    // Input stream.
    IStreamReader& reader_;

    // Input stream window (3 frames).
    core::CircularBuffer<ISampleBufferPtr, 3> window_;

    // Pointers to 3 frames of stream window.
    sample_t* prev_frame_;
    sample_t* curr_frame_;
    sample_t* next_frame_;

    // Frame size.
    // (frame_size_ / st_Nwindow) is maximum allowed scaling ratio.
    const size_t frame_size_;

    // Frame size in Q8.24.
    const fixedpoint_t qt_frame_size_;

    // Time position of output sample in terms of input samples indexes.
    // For example 0 -- time position of first sample in curr_frame_.
    fixedpoint_t qt_sample_;

    // Time distance between two output samples, equals to resampling factor.
    fixedpoint_t qt_dt_;

    // Resampling factor.
    float scaling_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_H_
