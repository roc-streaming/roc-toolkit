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

#ifdef ROC_TARGET_SOX
#include "roc_sndio/sox_backend.h"
#endif // ROC_TARGET_SOX

namespace roc {
namespace sndio {

BackendDispatcher::BackendDispatcher()
    : n_backends_(0) {
#ifdef ROC_TARGET_SOX
    add_backend_(SoxBackend::instance());
#endif // ROC_TARGET_SOX
}

void BackendDispatcher::set_frame_size(size_t frame_size) {
#ifdef ROC_TARGET_SOX
    SoxBackend::instance().set_frame_size(frame_size);
#endif // ROC_TARGET_SOX
}

ISink* BackendDispatcher::open_sink(core::IAllocator& allocator,
                                    const char* driver,
                                    const char* output,
                                    const Config& config) {
    IBackend* backend = select_backend_(driver, output, IBackend::ProbeSink);
    if (!backend) {
        return NULL;
    }
    return backend->open_sink(allocator, driver, output, config);
}

ISource* BackendDispatcher::open_source(core::IAllocator& allocator,
                                        const char* driver,
                                        const char* input,
                                        const Config& config) {
    IBackend* backend = select_backend_(driver, input, IBackend::ProbeSource);
    if (!backend) {
        return NULL;
    }
    return backend->open_source(allocator, driver, input, config);
}

IBackend*
BackendDispatcher::select_backend_(const char* driver, const char* inout, int flags) {
    if (IBackend* backend = probe_backends_(driver, inout, flags | IBackend::ProbeFile)) {
        return backend;
    }

    if (IBackend* backend = probe_backends_(
            driver, inout, flags | IBackend::ProbeFile | IBackend::ProbeDevice)) {
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
