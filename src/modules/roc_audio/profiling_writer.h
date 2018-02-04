/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/profiling_writer.h
//! @brief Profiling writer.

#ifndef ROC_AUDIO_PROFILING_WRITER_H_
#define ROC_AUDIO_PROFILING_WRITER_H_

#include "roc_audio/iwriter.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Profiling writer.
class ProfilingWriter : public IWriter, public core::NonCopyable<> {
public:
    //! Initialization.
    ProfilingWriter(IWriter& writer, packet::channel_mask_t channels, size_t sample_rate);

    //! Write audio frame.
    virtual void write(Frame& frame);

private:
    core::nanoseconds_t write_(Frame& frame);
    void update_(double speed);

    IWriter& writer_;

    core::RateLimiter rate_limiter_;

    double avg_speed_;
    double avg_len_;

    const size_t sample_rate_;
    const size_t num_channels_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PROFILING_WRITER_H_
