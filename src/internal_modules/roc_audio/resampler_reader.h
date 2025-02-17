/*
 * Copyright (c) 2018 Roc Streaming authors
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
#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_reader.h"
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

//! Resampler element for reading pipeline.
class ResamplerReader : public IFrameReader, public core::NonCopyable<> {
public:
    //! Initialize.
    ResamplerReader(IFrameReader& frame_reader,
                    FrameFactory& frame_factory,
                    IResampler& resampler,
                    const SampleSpec& in_spec,
                    const SampleSpec& out_spec);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Set new resample factor.
    bool set_scaling(float multiplier);

    //! Read audio frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(Frame& frame, packet::stream_timestamp_t duration, FrameReadMode mode);

private:
    status::StatusCode push_input_(FrameReadMode mode);
    core::nanoseconds_t capture_ts_(const Frame& out_frame);

    FrameFactory& frame_factory_;
    IFrameReader& frame_reader_;

    IResampler& resampler_;

    const SampleSpec in_spec_;
    const SampleSpec out_spec_;

    core::Slice<sample_t> in_buf_;
    size_t in_buf_pos_;
    FramePtr in_frame_;

    // timestamp of the last sample +1 of the last frame pushed into resampler
    core::nanoseconds_t last_in_cts_;

    float scaling_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_READER_H_
