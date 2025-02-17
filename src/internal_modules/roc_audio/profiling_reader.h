/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/profiling_reader.h
//! @brief Profiling reader.

#ifndef ROC_AUDIO_PROFILING_READER_H_
#define ROC_AUDIO_PROFILING_READER_H_

#include "roc_audio/iframe_reader.h"
#include "roc_audio/profiler.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Profiling reader.
class ProfilingReader : public IFrameReader, public core::NonCopyable<> {
public:
    //! Initialization.
    ProfilingReader(IFrameReader& reader,
                    core::IArena& arena,
                    const SampleSpec& sample_spec,
                    ProfilerConfig profiler_config);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Read audio frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(Frame& frame, packet::stream_timestamp_t duration, FrameReadMode mode);

private:
    core::nanoseconds_t read_(Frame& frame, bool& ret);

    Profiler profiler_;
    IFrameReader& reader_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PROFILING_READER_H_
