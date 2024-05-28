/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/channel_mapper_writer.h
//! @brief Channel mapper writer.

#ifndef ROC_AUDIO_CHANNEL_MAPPER_WRITER_H_
#define ROC_AUDIO_CHANNEL_MAPPER_WRITER_H_

#include "roc_audio/channel_mapper.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_writer.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

//! Channel mapper writer.
//! Maps samples to another channel mask and writes them to nested writer.
class ChannelMapperWriter : public IFrameWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    ChannelMapperWriter(IFrameWriter& frame_writer,
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

    ChannelMapper mapper_;

    const SampleSpec in_spec_;
    const SampleSpec out_spec_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_MAPPER_WRITER_H_
