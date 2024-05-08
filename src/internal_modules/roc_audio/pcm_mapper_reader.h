/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/pcm_mapper_reader.h
//! @brief Pcm mapper reader.

#ifndef ROC_AUDIO_PCM_MAPPER_READER_H_
#define ROC_AUDIO_PCM_MAPPER_READER_H_

#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/pcm_mapper.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

//! Pcm mapper reader.
//! Reads frames from nested reader and maps them to another pcm mask.
class PcmMapperReader : public IFrameReader, public core::NonCopyable<> {
public:
    //! Initialize.
    PcmMapperReader(IFrameReader& reader,
                    FrameFactory& frame_factory,
                    const SampleSpec& in_spec,
                    const SampleSpec& out_spec);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Read audio frame.
    virtual bool read(Frame& frame);

private:
    PcmMapper mapper_;

    IFrameReader& in_reader_;
    core::Slice<uint8_t> in_buf_;

    const SampleSpec in_spec_;
    const SampleSpec out_spec_;

    const size_t num_ch_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_MAPPER_READER_H_
