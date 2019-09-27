/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/log.h"
#include "roc_core/stddefs.h"
#include "roc_core/unique_ptr.h"
#include "roc_sndio/pulseaudio_backend.h"
#include "roc_sndio/pulseaudio_sink.h"

namespace roc {
namespace sndio {

PulseaudioBackend::PulseaudioBackend() {
    roc_log(LogDebug, "initializing pulseaudio backend");
}

bool PulseaudioBackend::probe(const char* driver, const char*, int filter_flags) {
    if ((filter_flags & FilterDevice) == 0) {
        return false;
    }

    if ((filter_flags & FilterSink) == 0) {
        return false;
    }

    return !driver || strcmp(driver, "pulse") == 0;
}

ISink* PulseaudioBackend::open_sink(core::IAllocator& allocator,
                                    const char*,
                                    const char* output,
                                    const Config& config) {
    core::UniquePtr<PulseaudioSink> sink(new (allocator) PulseaudioSink(config),
                                         allocator);
    if (!sink) {
        return NULL;
    }

    if (!sink->open(output)) {
        return NULL;
    }

    return sink.release();
}

ISource* PulseaudioBackend::open_source(core::IAllocator&,
                                        const char*,
                                        const char*,
                                        const Config&) {
    return NULL;
}

bool PulseaudioBackend::get_drivers(core::StringList& list, int filter_flags) {
    if (filter_flags & FilterDevice) {
        return list.push_back_uniq("pulse");
    }
    return true;
}

} // namespace sndio
} // namespace roc
