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
    register_backend_(PulseaudioBackend::instance());
#endif // ROC_TARGET_PULSEAUDIO
#ifdef ROC_TARGET_SOX
    register_backend_(SoxBackend::instance());
#endif // ROC_TARGET_SOX
}

void BackendDispatcher::set_frame_size(size_t frame_size) {
    (void)frame_size;
#ifdef ROC_TARGET_SOX
    SoxBackend::instance().set_frame_size(frame_size);
#endif // ROC_TARGET_SOX
}

ISink* BackendDispatcher::open_sink(core::IAllocator& allocator,
                                    const address::IoURI& uri,
                                    const char* force_format,
                                    const Config& config) {
    const int flags = select_driver_type_(uri) | IBackend::FilterSink;

    const char* driver = select_driver_name_(uri, force_format);
    const char* output = select_inout_(uri);

    IBackend* backend = select_backend_(driver, output, flags);
    if (!backend) {
        return NULL;
    }

    return backend->open_sink(allocator, driver, output, config);
}

ISource* BackendDispatcher::open_source(core::IAllocator& allocator,
                                        const address::IoURI& uri,
                                        const char* force_format,
                                        const Config& config) {
    const int flags = select_driver_type_(uri) | IBackend::FilterSource;

    const char* driver = select_driver_name_(uri, force_format);
    const char* input = select_inout_(uri);

    IBackend* backend = select_backend_(driver, input, flags);
    if (!backend) {
        return NULL;
    }

    return backend->open_source(allocator, driver, input, config);
}

bool BackendDispatcher::get_supported_schemes(core::StringList& list) {
    list.clear();

    for (size_t n = 0; n < n_backends_; n++) {
        // every device driver has its own scheme
        if (!backends_[n]->get_drivers(list, IBackend::FilterDevice)) {
            return false;
        }
    }

    // all file drivers has a single "file" scheme
    if (!list.push_back("file")) {
        return false;
    }

    return true;
}

bool BackendDispatcher::get_supported_formats(core::StringList& list) {
    list.clear();

    for (size_t n = 0; n < n_backends_; n++) {
        if (!backends_[n]->get_drivers(list, IBackend::FilterFile)) {
            return false;
        }
    }

    return true;
}

int BackendDispatcher::select_driver_type_(const address::IoURI& uri) const {
    if (uri.is_file()) {
        return IBackend::FilterFile;
    } else {
        return IBackend::FilterDevice;
    }
}

const char* BackendDispatcher::select_driver_name_(const address::IoURI& uri,
                                                   const char* force_format) const {
    if (uri.is_file()) {
        if (force_format && *force_format) {
            // use specific file driver
            return force_format;
        }
        // auto-detect file driver
        return NULL;
    }

    if (!uri.is_empty()) {
        // use spcific device driver
        return uri.scheme;
    }

    // use default device driver
    return NULL;
}

const char* BackendDispatcher::select_inout_(const address::IoURI& uri) const {
    if (uri.is_empty()) {
        return NULL;
    } else {
        return uri.path;
    }
}

IBackend*
BackendDispatcher::select_backend_(const char* driver, const char* inout, int flags) {
    for (size_t n = 0; n < n_backends_; n++) {
        if (backends_[n]->probe(driver, inout, flags)) {
            return backends_[n];
        }
    }
    return NULL;
}

void BackendDispatcher::register_backend_(IBackend& backend) {
    roc_panic_if(n_backends_ == MaxBackends);
    backends_[n_backends_++] = &backend;
}

} // namespace sndio
} // namespace roc
