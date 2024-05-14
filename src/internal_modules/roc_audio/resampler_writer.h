/*
 * Copyright (c) 2018 Roc Streaming authors
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
#include "roc_audio/iframe_writer.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/array.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Resampler element for writing pipeline.
class ResamplerWriter : public IFrameWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    ResamplerWriter(IFrameWriter& writer,
                    IResampler& resampler,
                    core::BufferFactory& buffer_factory,
                    const SampleSpec& in_sample_spec,
                    const SampleSpec& out_sample_spec);

    //! Check if object is successfully constructed.
    bool is_valid() const;

    //! Set new resample factor.
    bool set_scaling(float multiplier);

    //! Read audio frame.
    virtual void write(Frame&);

private:
    size_t push_input_(Frame& in_frame, size_t in_pos);
    core::nanoseconds_t capture_ts_(Frame& in_frame, size_t in_pos);

    IResampler& resampler_;
    IFrameWriter& writer_;

    const audio::SampleSpec in_sample_spec_;
    const audio::SampleSpec out_sample_spec_;

    core::Slice<sample_t> input_buf_;
    core::Slice<sample_t> output_buf_;

    size_t input_buf_pos_;
    size_t output_buf_pos_;

    float scaling_;
    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_WRITER_H_
