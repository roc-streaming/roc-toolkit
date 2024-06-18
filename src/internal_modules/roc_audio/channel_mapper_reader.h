/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/channel_mapper_reader.h
//! @brief Channel mapper reader.

#ifndef ROC_AUDIO_CHANNEL_MAPPER_READER_H_
#define ROC_AUDIO_CHANNEL_MAPPER_READER_H_

#include "roc_audio/channel_mapper.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

//! Channel mapper reader.
//! Reads frames from nested reader and maps them to another channel mask.
class ChannelMapperReader : public IFrameReader, public core::NonCopyable<> {
public:
    //! Initialize.
    ChannelMapperReader(IFrameReader& frame_reader,
                        FrameFactory& frame_factory,
                        const SampleSpec& in_spec,
                        const SampleSpec& out_spec);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Read audio frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(Frame& frame, packet::stream_timestamp_t duration, FrameReadMode mode);

private:
    FrameFactory& frame_factory_;
    IFrameReader& frame_reader_;

    FramePtr in_frame_;

    ChannelMapper mapper_;

    const SampleSpec in_spec_;
    const SampleSpec out_spec_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_MAPPER_READER_H_
