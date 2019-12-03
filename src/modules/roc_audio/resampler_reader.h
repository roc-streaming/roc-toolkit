/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/resampler_reader.h
//! @brief Resampler.

#ifndef ROC_AUDIO_RESAMPLER_READER_H_
#define ROC_AUDIO_RESAMPLER_READER_H_

#include "roc_audio/frame.h"
#include "roc_audio/ireader.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/units.h"
#include "roc_core/array.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Resamples audio stream with non-integer dynamically changing factor.
//! @remarks
//!  Typicaly being used with factor close to 1 ( 0.9 < factor < 1.1 ).
class ResamplerReader : public IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader specifies input audio stream used in read()
    //!  - @p resampler interface that hides which resampler algorithm will be used
    //!  - @p frame_size is number of samples per resampler frame per audio channel
    ResamplerReader(IReader& reader,
                    IResampler& resampler,
                    core::BufferPool<sample_t>& buffer_pool,
                    size_t frame_size);

    //! Check if object is successfully constructed.
    bool valid() const;

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
    bool init_frames_(core::BufferPool<sample_t>&);
    void renew_frames_();

    IResampler& resampler_;
    IReader& reader_;

    core::Slice<sample_t> frames_[3];
    const size_t frame_size_;
    bool frames_empty_;

    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_READER_H_
