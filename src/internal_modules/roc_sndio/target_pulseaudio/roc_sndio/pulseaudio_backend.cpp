/*
 * Copyright (c) 2019 Roc Streaming authors
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
    roc_log(LogDebug, "pulseaudio backend: initializing");
}

ITerminal* PulseaudioBackend::open_terminal(core::IAllocator& allocator,
                                            TerminalType terminal_type,
                                            DriverType driver_type,
                                            const char* driver,
                                            const char* path,
                                            const Config& config) {
    if (driver_type != DriverType_Device) {
        return NULL;
    }

    if (driver && strcmp(driver, "pulse") != 0) {
        return NULL;
    }

    switch (terminal_type) {
    case Terminal_Sink: {
        core::ScopedPtr<PulseaudioSink> sink(new (allocator) PulseaudioSink(config),
                                             allocator);
        if (!sink) {
            roc_log(LogDebug, "pulseaudio backend: can't construct sink: path=%s", path);
            return NULL;
        }

        if (!sink->open(path)) {
            roc_log(LogDebug, "pulseaudio backend: can't open sink: path=%s", path);
            return NULL;
        }

        return sink.release();
    } break;

    case Terminal_Source: {
        return NULL;
    } break;

    default:
        break;
    }

    roc_panic("pulseaudio backend: invalid terminal type");
}

bool PulseaudioBackend::get_drivers(core::Array<DriverInfo>& driver_list) {
    if (!driver_list.grow_exp(driver_list.size() + 1)) {
        return false;
    }

    driver_list.push_back(DriverInfo("pulse", DriverType_Device,
                                     DriverFlag_IsDefault | DriverFlag_SupportsSink,
                                     this));

    return true;
}

} // namespace sndio
} // namespace roc
