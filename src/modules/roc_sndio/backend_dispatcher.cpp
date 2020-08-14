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

namespace {

int select_driver_type(const address::IoURI& uri) {
    if (uri.is_file()) {
        return IBackend::FilterFile;
    } else {
        return IBackend::FilterDevice;
    }
}

const char* select_driver_name(const address::IoURI& uri, const char* force_format) {
    if (uri.is_file()) {
        if (force_format && *force_format) {
            // use specific file driver
            return force_format;
        }
        // auto-detect file driver
        return NULL;
    }

    if (uri.is_valid()) {
        // use specific device driver
        return uri.scheme();
    }

    // use default device driver
    return NULL;
}

const char* select_input_output(const address::IoURI& uri) {
    if (uri.is_valid()) {
        return uri.path();
    } else {
        return NULL;
    }
}

bool check_opened(core::StringList& list, const char* driver) {
    const char* tried_driver = list.front();
    while (tried_driver) {
        if (strcmp(tried_driver, driver) == 0) {
            return true;
        }
        tried_driver = list.nextof(tried_driver);
    }
    return false;
}

} // namespace

BackendDispatcher::BackendDispatcher(core::IAllocator& allocator)
    : allocator_(allocator)
    , driver_info_list_(allocator_)
    , n_backends_(0) {
#ifdef ROC_TARGET_PULSEAUDIO
    register_backend_(PulseaudioBackend::instance());
#endif // ROC_TARGET_PULSEAUDIO
#ifdef ROC_TARGET_SOX
    register_backend_(SoxBackend::instance());
#endif // ROC_TARGET_SOX
    init_driver_info_();
}

void BackendDispatcher::set_frame_size(core::nanoseconds_t frame_length,
                                       size_t sample_rate,
                                       packet::channel_mask_t channels) {
#ifdef ROC_TARGET_SOX
    SoxBackend::instance().set_frame_size(frame_length, sample_rate, channels);
#endif // ROC_TARGET_SOX
    (void)frame_length;
    (void)sample_rate;
    (void)channels;
}

ISink* BackendDispatcher::open_sink(const address::IoURI& uri,
                                    const char* force_format,
                                    const Config& config) {
    const int flags = select_driver_type(uri);

    const char* driver = select_driver_name(uri, force_format);
    const char* output = select_input_output(uri);

    if (!driver && !output) {
        core::StringList tried_open(allocator_);
        for (size_t n = 0; n < driver_info_list_.size(); n++) {
            driver = driver_info_list_[n].name;
            IBackend* backend = driver_info_list_[n].backend;
            unsigned int driver_flags = driver_info_list_[n].flags;

            if (check_opened(tried_open, driver)) {
                continue;
            }

            if ((driver_flags & DriverDefault) && (driver_flags & DriverSink)) {
                ISink* sink =
                    backend->open_sink(allocator_, driver, "default", config, flags);
                tried_open.push_back_unique(driver);
                if (sink) {
                    return sink;
                }
            }
        }
    } else {
        if (!output) {
            roc_panic("invalid driver and device combination");
        }
        IBackend* backend = get_backend_(driver, DriverSink);
        if (!backend) {
            roc_log(LogError, "driver not supported by available backends");
            return NULL;
        }
        ISink* sink = backend->open_sink(allocator_, driver, output, config, flags);
        if (sink) {
            return sink;
        }
        roc_log(LogError,
                "BackendDispatcher: open_sink() failed for driver=(%s) and output=(%s)",
                driver, output);
    }
    return NULL;
}

ISource* BackendDispatcher::open_source(const address::IoURI& uri,
                                        const char* force_format,
                                        const Config& config) {
    const int flags = select_driver_type(uri);

    const char* driver = select_driver_name(uri, force_format);
    const char* input = select_input_output(uri);

    if (!driver && !input) {
        core::StringList tried_open(allocator_);
        for (size_t n = 0; n < driver_info_list_.size(); n++) {
            driver = driver_info_list_[n].name;
            IBackend* backend = driver_info_list_[n].backend;
            unsigned int driver_flags = driver_info_list_[n].flags;

            if (check_opened(tried_open, driver)) {
                continue;
            }

            if ((driver_flags & DriverDefault) && (driver_flags & DriverSource)) {
                roc_log(LogDebug, "Trying driver: (%s)", driver);
                ISource* source =
                    backend->open_source(allocator_, driver, "default", config, flags);
                tried_open.push_back_unique(driver);
                if (source) {
                    return source;
                }
            }
        }
    } else {
        if (!input) {
            roc_panic("invalid driver and device combination");
        }
        IBackend* backend = get_backend_(driver, DriverSource);
        if (!backend) {
            roc_log(LogError, "driver not supported by available backends");
            return NULL;
        }
        ISource* source = backend->open_source(allocator_, driver, input, config, flags);
        if (source) {
            return source;
        }
        roc_log(LogError,
                "BackendDispatcher: open_sink() failed for driver=(%s) and output=(%s)",
                driver, input);
    }
    return NULL;
}

bool BackendDispatcher::get_supported_schemes(core::StringList& list) {
    list.clear();

    for (size_t n = 0; n < driver_info_list_.size(); n++) {
        // every device driver has its own scheme
        if (driver_info_list_[n].flags & DriverDevice) {
            if (!list.push_back_unique(driver_info_list_[n].name)) {
                return false;
            }
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

    for (size_t n = 0; n < driver_info_list_.size(); n++) {
        if (driver_info_list_[n].flags & DriverFile) {
            if (!list.push_back_unique(driver_info_list_[n].name)) {
                return false;
            }
        }
    }
    return true;
}

void BackendDispatcher::register_backend_(IBackend& backend) {
    roc_panic_if(n_backends_ == MaxBackends);
    backends_[n_backends_++] = &backend;
}

void BackendDispatcher::init_driver_info_() {
    if (!driver_info_list_.grow(MaxDrivers)) {
        roc_panic("BackendDispatcher: driver_info_list_ could not grow");
    }
    for (size_t n = 0; n < n_backends_; n++) {
        backends_[n]->get_drivers(driver_info_list_,
                                  IBackend::FilterDevice | IBackend::FilterFile);
    }
    roc_log(LogDebug, "initialized driver_info_list_ size=(%zu)",
            driver_info_list_.size());
}

IBackend* BackendDispatcher::get_backend_(const char* driver, unsigned int driver_flags) {
    if (!driver) {
#ifdef ROC_TARGET_SOX
        return &SoxBackend::instance();
#endif // ROC_TARGET_SOX
        return NULL;
    }
    for (size_t n = 0; n < driver_info_list_.size(); n++) {
        if (driver_flags & driver_info_list_[n].flags) {
            if (strcmp(driver, driver_info_list_[n].name) == 0) {
                return driver_info_list_[n].backend;
            }
        }
    }
    return NULL;
}

} // namespace sndio
} // namespace roc
