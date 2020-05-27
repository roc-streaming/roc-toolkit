/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/resampler_writer.h
//! @brief Resampler.

#ifndef ROC_AUDIO_RESAMPLER_WRITER_H_
#define ROC_AUDIO_RESAMPLER_WRITER_H_

#include "roc_audio/frame.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/iwriter.h"
#include "roc_audio/units.h"
#include "roc_core/array.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Resampler element for writing pipeline.
class ResamplerWriter : public IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    ResamplerWriter(IWriter& writer,
                    IResampler& resampler,
                    core::BufferPool<sample_t>& buffer_pool,
                    core::nanoseconds_t frame_length,
                    size_t sample_rate,
                    packet::channel_mask_t channels);

    //! Check if object is successfully constructed.
    bool valid() const;

    //! Set new resample factor.
    bool set_scaling(size_t input_rate, size_t output_rate, float multiplier);

    //! Read audio frame.
    virtual void write(Frame&);

private:
    size_t push_input_(Frame& frame, size_t frame_pos);

    IResampler& resampler_;
    IWriter& writer_;

    size_t input_pos_;
    size_t output_pos_;

    core::Slice<sample_t> input_;
    core::Slice<sample_t> output_;

    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_WRITER_H_
