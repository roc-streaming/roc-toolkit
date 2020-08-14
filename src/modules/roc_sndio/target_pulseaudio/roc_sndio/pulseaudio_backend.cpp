/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"
#include "roc_sndio/pulseaudio_backend.h"
#include "roc_sndio/pulseaudio_sink.h"

namespace roc {
namespace sndio {

PulseaudioBackend::PulseaudioBackend() {
    roc_log(LogDebug, "initializing pulseaudio backend");
}

ISink* PulseaudioBackend::open_sink(core::IAllocator& allocator,
                                    const char* driver,
                                    const char* output,
                                    const Config& config,
                                    int filter_flags) {
    if ((filter_flags & FilterDevice) == 0) {
        return NULL;
    }

    if (driver && strcmp(driver, "pulse")) {
        return NULL;
    }

    core::ScopedPtr<PulseaudioSink> sink(new (allocator) PulseaudioSink(config),
                                         allocator);

    if (!sink) {
        return NULL;
    }

    if (!sink->open(output)) {
        return NULL;
    }

    return sink.release();
}

ISource* PulseaudioBackend::open_source(
    core::IAllocator&, const char*, const char*, const Config&, int) {
    return NULL;
}

bool PulseaudioBackend::get_drivers(core::Array<DriverInfo>& list, int filter_flags) {
    if (filter_flags & FilterDevice) {
        DriverInfo driver_info;
        driver_info.set("pulse", this, DriverDevice | DriverDefault | DriverSink);
        list.push_back(driver_info);
    }
    return true;
}

} // namespace sndio
} // namespace roc
