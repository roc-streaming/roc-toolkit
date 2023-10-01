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
    ResamplerReader(IFrameReader& reader,
                    IResampler& resampler,
                    const SampleSpec& in_sample_spec,
                    const SampleSpec& out_sample_spec);

    //! Check if object is successfully constructed.
    bool is_valid() const;

    //! Set new resample factor.
    bool set_scaling(float multiplier);

    //! Read audio frame.
    virtual bool read(Frame&);

private:
    bool push_input_();
    core::nanoseconds_t capture_ts_(Frame& out_frame);

    IResampler& resampler_;
    IFrameReader& reader_;

    const audio::SampleSpec in_sample_spec_;
    const audio::SampleSpec out_sample_spec_;

    // timestamp of the last sample +1 of the last frame pushed into resampler
    core::nanoseconds_t last_in_cts_;

    float scaling_;
    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_READER_H_
