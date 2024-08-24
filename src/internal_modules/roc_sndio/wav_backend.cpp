/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_sndio/wav_backend.h"
#include "roc_sndio/wav_sink.h"
#include "roc_sndio/wav_source.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

WavBackend::WavBackend() {
}

const char* WavBackend::name() const {
    return "wav";
}

bool WavBackend::discover_drivers(core::Array<DriverInfo, MaxDrivers>& result) {
    if (!result.push_back(DriverInfo(
            "file", Driver_File | Driver_SupportsSink | Driver_SupportsSource, this))) {
        return false;
    }
    return true;
}

bool WavBackend::discover_formats(core::Array<FormatInfo, MaxFormats>& result) {
    if (!result.push_back(FormatInfo(
            "file", "wav", Driver_File | Driver_SupportsSink | Driver_SupportsSource,
            this))) {
        return false;
    }
    return true;
}

bool WavBackend::discover_subformat_groups(core::StringList& result) {
    // no sub-formats except pcm
    return true;
}

bool WavBackend::discover_subformats(const char* group, core::StringList& result) {
    // no sub-formats except pcm
    return true;
}

status::StatusCode WavBackend::open_device(DeviceType device_type,
                                           const char* driver,
                                           const char* path,
                                           const IoConfig& io_config,
                                           audio::FrameFactory& frame_factory,
                                           core::IArena& arena,
                                           IDevice** result) {
    roc_panic_if(!driver);
    roc_panic_if(!path);

    if (strcmp(driver, "file") != 0) {
        // Not file://, go to next backend.
        return status::StatusNoDriver;
    }

    switch (device_type) {
    case DeviceType_Sink: {
        core::ScopedPtr<WavSink> sink(new (arena)
                                          WavSink(frame_factory, arena, io_config, path));

        if (!sink) {
            roc_log(LogDebug, "wav backend: can't allocate sink: path=%s", path);
            return status::StatusNoMem;
        }

        if (sink->init_status() != status::StatusOK) {
            roc_log(LogDebug, "wav backend: can't open sink: path=%s status=%s", path,
                    status::code_to_str(sink->init_status()));
            return sink->init_status();
        }

        *result = sink.hijack();
        return status::StatusOK;
    } break;

    case DeviceType_Source: {
        core::ScopedPtr<WavSource> source(
            new (arena) WavSource(frame_factory, arena, io_config, path));

        if (!source) {
            roc_log(LogDebug, "wav backend: can't allocate source: path=%s", path);
            return status::StatusNoMem;
        }

        if (source->init_status() != status::StatusOK) {
            roc_log(LogDebug, "wav backend: can't open source: path=%s status=%s", path,
                    status::code_to_str(source->init_status()));
            return source->init_status();
        }

        *result = source.hijack();
        return status::StatusOK;
    } break;
    }

    roc_panic("wav backend: invalid device type");
}

} // namespace sndio
} // namespace roc
