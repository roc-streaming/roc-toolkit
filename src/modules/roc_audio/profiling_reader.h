/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/profiling_reader.h
//! @brief Profiling reader.

#ifndef ROC_AUDIO_PROFILING_READER_H_
#define ROC_AUDIO_PROFILING_READER_H_

#include "roc_audio/ireader.h"
#include "roc_audio/profiler.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Profiling reader.
class ProfilingReader : public IReader, public core::NonCopyable<> {
public:
    //! Initialization.
    ProfilingReader(IReader& reader,
                    core::IAllocator& allocator,
                    packet::channel_mask_t channels,
                    size_t sample_rate,
                    ProfilerConfig profiler_config);

    //! Read audio frame.
    virtual bool read(Frame& frame);

    //! Check if the profiler was succefully constructed.
    bool valid() const;

private:
    core::nanoseconds_t read_(Frame& frame, bool& ret);

    Profiler profiler_;
    IReader& reader_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PROFILING_READER_H_
