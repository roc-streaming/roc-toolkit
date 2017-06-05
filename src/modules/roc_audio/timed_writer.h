/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/timed_writer.h
//! @brief Timed writer.

#ifndef ROC_AUDIO_TIMED_WRITER_H_
#define ROC_AUDIO_TIMED_WRITER_H_

#include "roc_config/config.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

#include "roc_audio/isample_buffer_writer.h"

namespace roc {
namespace audio {

//! Timed writer.
//! @remarks
//!  Constrains writing speed to specified sample rate.
class TimedWriter : public ISampleBufferWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p output is output sample writer;
    //!  - @p channels is bitmask of enabled channels;
    //!  - @p rate is constraining sample rate.
    //!
    //! TimedWriter ensures that no more than @p rate samples per second
    //! are passed to output writer.
    explicit TimedWriter(
        ISampleBufferWriter& output,
        packet::channel_mask_t channels = ROC_CONFIG_DEFAULT_CHANNEL_MASK,
        size_t rate = ROC_CONFIG_DEFAULT_SAMPLE_RATE);

    //! Write buffer.
    //! @remarks
    //!  Sleeps appropriate amount of time and passes @p buffer to
    //!  output writer.
    virtual void write(const ISampleBufferConstSlice& buffer);

private:
    ISampleBufferWriter& output_;
    const uint64_t rate_;

    uint64_t n_samples_;
    uint64_t start_ms_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TIMED_WRITER_H_
