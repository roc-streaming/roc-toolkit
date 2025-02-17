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
#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_writer.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/array.h"
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
    ResamplerWriter(IFrameWriter& frame_writer,
                    FrameFactory& frame_factory,
                    IResampler& resampler,
                    const SampleSpec& in_sample_spec,
                    const SampleSpec& out_sample_spec);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Set new resample factor.
    bool set_scaling(float multiplier);

    //! Write audio frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(Frame& frame);

private:
    status::StatusCode write_output_(const Frame& in_frame, size_t in_frame_pos);
    size_t push_input_(Frame& in_frame, size_t in_frame_pos);
    core::nanoseconds_t capture_ts_(const Frame& in_frame, size_t in_frame_pos);

    FrameFactory& frame_factory_;
    IFrameWriter& frame_writer_;

    IResampler& resampler_;

    const SampleSpec in_spec_;
    const SampleSpec out_spec_;

    core::Slice<sample_t> in_buf_;
    size_t in_buf_pos_;

    FramePtr out_frame_;
    size_t out_frame_pos_;

    float scaling_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_WRITER_H_
