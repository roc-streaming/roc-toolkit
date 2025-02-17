/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/profiling_writer.h
//! @brief Profiling writer.

#ifndef ROC_AUDIO_PROFILING_WRITER_H_
#define ROC_AUDIO_PROFILING_WRITER_H_

#include "roc_audio/iframe_writer.h"
#include "roc_audio/profiler.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Profiling writer.
class ProfilingWriter : public IFrameWriter, public core::NonCopyable<> {
public:
    //! Initialization.
    ProfilingWriter(IFrameWriter& writer,
                    core::IArena& arena,
                    const SampleSpec& sample_spec,
                    ProfilerConfig profiler_config);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Write audio frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(Frame& frame);

private:
    Profiler profiler_;
    IFrameWriter& writer_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PROFILING_WRITER_H_
