/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/backend_dispatcher.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

#ifdef ROC_TARGET_PULSEAUDIO
#include "roc_sndio/pulseaudio_backend.h"
#endif // ROC_TARGET_PULSEAUDIO

#ifdef ROC_TARGET_SOX
#include "roc_sndio/sox_backend.h"
#endif // ROC_TARGET_SOX

namespace roc {
namespace sndio {

BackendDispatcher::BackendDispatcher()
    : n_backends_(0) {
#ifdef ROC_TARGET_PULSEAUDIO
    add_backend_(PulseaudioBackend::instance());
#endif // ROC_TARGET_PULSEAUDIO
#ifdef ROC_TARGET_SOX
    add_backend_(SoxBackend::instance());
#endif // ROC_TARGET_SOX
}

void BackendDispatcher::set_frame_size(size_t frame_size) {
    (void)frame_size;
#ifdef ROC_TARGET_SOX
    SoxBackend::instance().set_frame_size(frame_size);
#endif // ROC_TARGET_SOX
}

ISink* BackendDispatcher::open_sink(core::IAllocator& allocator,
                                    const char* driver,
                                    const char* output,
                                    const Config& config) {
    IBackend* backend = select_backend_(driver, output, IBackend::FilterSink);
    if (!backend) {
        return NULL;
    }
    return backend->open_sink(allocator, driver, output, config);
}

ISource* BackendDispatcher::open_source(core::IAllocator& allocator,
                                        const char* driver,
                                        const char* input,
                                        const Config& config) {
    IBackend* backend = select_backend_(driver, input, IBackend::FilterSource);
    if (!backend) {
        return NULL;
    }
    return backend->open_source(allocator, driver, input, config);
}

bool BackendDispatcher::get_device_drivers(core::Array<DriverInfo>& arr) {
    arr.resize(0);
    for (size_t n = 0; n < n_backends_; n++) {
        if (!backends_[n]->get_drivers(arr, IBackend::FilterDevice)) {
            return false;
        }
    }
    return true;
}

bool BackendDispatcher::get_file_drivers(core::Array<DriverInfo>& arr) {
    arr.resize(0);
    for (size_t n = 0; n < n_backends_; n++) {
        if (!backends_[n]->get_drivers(arr, IBackend::FilterFile)) {
            return false;
        }
    }
    return true;
}

IBackend*
BackendDispatcher::select_backend_(const char* driver, const char* inout, int flags) {
    if (IBackend* backend =
            probe_backends_(driver, inout, flags | IBackend::FilterFile)) {
        return backend;
    }

    if (IBackend* backend = probe_backends_(
            driver, inout, flags | IBackend::FilterFile | IBackend::FilterDevice)) {
        return backend;
    }

    roc_log(LogError, "no backend fround: driver=%s inout=%s", driver, inout);
    return NULL;
}

IBackend*
BackendDispatcher::probe_backends_(const char* driver, const char* inout, int flags) {
    for (size_t n = 0; n < n_backends_; n++) {
        if (backends_[n]->probe(driver, inout, flags)) {
            return backends_[n];
        }
    }
    return NULL;
}

void BackendDispatcher::add_backend_(IBackend& backend) {
    roc_panic_if(n_backends_ == MaxBackends);
    backends_[n_backends_++] = &backend;
}

} // namespace sndio
} // namespace roc
