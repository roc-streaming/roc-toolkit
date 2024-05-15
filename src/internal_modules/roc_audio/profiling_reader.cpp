/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/profiling_reader.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

ProfilingReader::ProfilingReader(IFrameReader& reader,
                                 core::IArena& arena,
                                 const SampleSpec& sample_spec,
                                 ProfilerConfig profiler_config)
    : profiler_(arena, sample_spec, profiler_config)
    , reader_(reader) {
}

bool ProfilingReader::is_valid() const {
    return profiler_.is_valid();
}

bool ProfilingReader::read(Frame& frame) {
    bool ret;
    const core::nanoseconds_t elapsed = read_(frame, ret);

    if (ret) {
        profiler_.add_frame(frame.duration(), elapsed);
    }
    return ret;
}

core::nanoseconds_t ProfilingReader::read_(Frame& frame, bool& ret) {
    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    ret = reader_.read(frame);

    return core::timestamp(core::ClockMonotonic) - start;
}

} // namespace audio
} // namespace roc
