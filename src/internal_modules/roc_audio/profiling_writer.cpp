/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/profiling_writer.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

ProfilingWriter::ProfilingWriter(IFrameWriter& writer,
                                 core::IArena& arena,
                                 const audio::SampleSpec& sample_spec,
                                 ProfilerConfig profiler_config)
    : profiler_(arena, sample_spec, profiler_config)
    , writer_(writer) {
}

bool ProfilingWriter::is_valid() const {
    return profiler_.is_valid();
}

void ProfilingWriter::write(Frame& frame) {
    const core::nanoseconds_t elapsed = write_(frame);

    profiler_.add_frame(frame.duration(), elapsed);
}

core::nanoseconds_t ProfilingWriter::write_(Frame& frame) {
    const core::nanoseconds_t start = core::timestamp(core::ClockMonotonic);

    writer_.write(frame);

    return core::timestamp(core::ClockMonotonic) - start;
}

} // namespace audio
} // namespace roc
