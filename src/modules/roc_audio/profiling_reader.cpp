/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/profiling_reader.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

ProfilingReader::ProfilingReader(IReader& reader,
                                 core::IAllocator& allocator,
                                 packet::channel_mask_t channels,
                                 size_t sample_rate,
                                 core::nanoseconds_t interval)
    : profiler_(allocator, channels, sample_rate, interval)
    , reader_(reader) {
}

void ProfilingReader::read(Frame& frame) {
    profiler_.begin_frame(frame.size());

    const core::nanoseconds_t elapsed = read_(frame);

    profiler_.end_frame(frame.size(), elapsed);
}

core::nanoseconds_t ProfilingReader::read_(Frame& frame) {
    const core::nanoseconds_t start = core::timestamp();

    reader_.read(frame);

    return core::timestamp() - start;
}

} // namespace audio
} // namespace roc
