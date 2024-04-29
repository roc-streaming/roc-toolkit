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
                                 const SampleSpec& sample_spec,
                                 ProfilerConfig profiler_config)
    : profiler_(arena, sample_spec, profiler_config)
    , writer_(writer) {
}

status::StatusCode ProfilingWriter::init_status() const {
    return profiler_.init_status();
}

status::StatusCode ProfilingWriter::write(Frame& frame) {
    const core::nanoseconds_t started = core::timestamp(core::ClockMonotonic);
    const status::StatusCode code = writer_.write(frame);
    const core::nanoseconds_t elapsed = core::timestamp(core::ClockMonotonic) - started;

    if (code == status::StatusOK) {
        profiler_.add_frame(frame.duration(), elapsed);
    }

    return code;
}

} // namespace audio
} // namespace roc
