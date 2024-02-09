/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/pcm_mapper_writer.h
//! @brief Pcm mapper writer.

#ifndef ROC_AUDIO_PCM_MAPPER_WRITER_H_
#define ROC_AUDIO_PCM_MAPPER_WRITER_H_

#include "roc_audio/iframe_writer.h"
#include "roc_audio/pcm_mapper.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

//! Pcm mapper writer.
//! Reads frames from nested writer and maps them to another pcm mask.
class PcmMapperWriter : public IFrameWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    PcmMapperWriter(IFrameWriter& writer,
                    core::BufferFactory<uint8_t>& buffer_factory,
                    const SampleSpec& in_spec,
                    const SampleSpec& out_spec);

    //! Check if the object was succefully constructed.
    bool is_valid() const;

    //! Read audio frame.
    virtual void write(Frame& frame);

private:
    PcmMapper mapper_;

    IFrameWriter& out_writer_;
    core::Slice<uint8_t> out_buf_;

    const SampleSpec in_spec_;
    const SampleSpec out_spec_;

    const size_t num_ch_;

    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_MAPPER_WRITER_H_
