/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/delayed_writer.h
//! @brief Delayed writer.

#ifndef ROC_AUDIO_DELAYED_WRITER_H_
#define ROC_AUDIO_DELAYED_WRITER_H_

#include "roc_config/config.h"
#include "roc_core/array.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

#include "roc_audio/isample_buffer_writer.h"

namespace roc {
namespace audio {

//! Delayed writer.
//! @remarks
//!  Delays writing output buffers until enough buffers are queued.
class DelayedWriter : public ISampleBufferWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p output is output sample writer;
    //!  - @p channels is bitmask of enabled channels;
    //!  - @p latency is number of samples to be queued before starting output.
    explicit DelayedWriter(
        ISampleBufferWriter& output,
        packet::channel_mask_t channels = ROC_CONFIG_DEFAULT_CHANNEL_MASK,
        size_t latency = ROC_CONFIG_DEFAULT_OUTPUT_LATENCY);

    //! Write buffer.
    virtual void write(const ISampleBufferConstSlice& buffer);

private:
    enum { MaxBuffers = ROC_CONFIG_MAX_SAMPLE_BUFFERS };

    ISampleBufferWriter& output_;
    const size_t n_ch_;
    const size_t latency_;
    size_t pending_;
    bool flushed_;

    core::Array<ISampleBufferConstSlice, MaxBuffers> queue_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_DELAYED_WRITER_H_
