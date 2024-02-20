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
#include "roc_audio/iframe_writer.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

//! Channel mapper writer.
//! Reads frames from nested writer and maps them to another channel mask.
class ChannelMapperWriter : public IFrameWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    ChannelMapperWriter(IFrameWriter& writer,
                        core::BufferFactory<sample_t>& buffer_factory,
                        const SampleSpec& in_spec,
                        const SampleSpec& out_spec);

    //! Check if the object was succefully constructed.
    bool is_valid() const;

    //! Write audio frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(Frame& frame);

private:
    void write_(sample_t* in_samples,
                size_t n_samples,
                unsigned flags,
                core::nanoseconds_t capture_ts);

    IFrameWriter& output_writer_;
    core::Slice<sample_t> output_buf_;

    ChannelMapper mapper_;

    const SampleSpec in_spec_;
    const SampleSpec out_spec_;

    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_MAPPER_WRITER_H_
