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

//! Resampler element for reading pipeline.
class ResamplerReader : public IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    ResamplerReader(IReader& reader, IResampler& resampler);

    //! Check if object is successfully constructed.
    bool valid() const;

    //! Set new resample factor.
    bool set_scaling(size_t input_rate, size_t output_rate, float multiplier);

    //! Read audio frame.
    virtual bool read(Frame&);

private:
    bool push_input_();

    IResampler& resampler_;
    IReader& reader_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RESAMPLER_READER_H_
