/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/pcm_mapper_writer.h
//! @brief PCM mapper writer.

#ifndef ROC_AUDIO_PCM_MAPPER_WRITER_H_
#define ROC_AUDIO_PCM_MAPPER_WRITER_H_

#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_writer.h"
#include "roc_audio/pcm_mapper.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

//! PCM mapper writer.
//! Maps frames to another PCM format and writes them to nested writer.
//! @remarks
//!  - Either input or output format must be raw samples (PcmSubformat_Raw).
//!  - Both input and output formats must be byte-aligned.
class PcmMapperWriter : public IFrameWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    PcmMapperWriter(IFrameWriter& frame_writer,
                    FrameFactory& frame_factory,
                    const SampleSpec& in_spec,
                    const SampleSpec& out_spec);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Write audio frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(Frame& frame);

private:
    FrameFactory& frame_factory_;
    IFrameWriter& frame_writer_;

    FramePtr out_frame_;

    core::Optional<PcmMapper> mapper_;

    const SampleSpec in_spec_;
    const SampleSpec out_spec_;

    const size_t num_ch_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_MAPPER_WRITER_H_
