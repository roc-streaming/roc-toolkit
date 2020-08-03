/*
 * Copyright (c) 2019 Roc Streaming authors
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

DriverType select_driver_type(const address::IoURI& uri) {
    if (uri.is_file()) {
        return DriverType_File;
    } else {
        return DriverType_Device;
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

    // use specific device driver
    return uri.scheme();
}

bool match_driver(const DriverInfo& driver_info,
                  const char* driver_name,
                  DriverType driver_type,
                  unsigned driver_flags) {
    if (driver_name != NULL && strcmp(driver_info.name, driver_name) != 0) {
        return false;
    }

    if (driver_info.type != driver_type) {
        return false;
    }

    if ((driver_info.flags & driver_flags) == 0) {
        return false;
    }

    return true;
}

} // namespace

BackendDispatcher::BackendDispatcher(core::IAllocator& allocator)
    : allocator_(allocator)
    , n_backends_(0)
    , drivers_(allocator_) {
    register_backends_();
    discover_drivers_();
}

void BackendDispatcher::set_frame_size(core::nanoseconds_t frame_length,
                                       const audio::SampleSpec& sample_spec) {
#ifdef ROC_TARGET_SOX
    SoxBackend::instance().set_frame_size(frame_length, sample_spec);
#endif // ROC_TARGET_SOX
    (void)frame_length;
    (void)sample_spec;
}

ISink* BackendDispatcher::open_default_sink(const Config& config) {
    return (ISink*)open_default_terminal_(Terminal_Sink, config);
}

ISource* BackendDispatcher::open_default_source(const Config& config) {
    return (ISource*)open_default_terminal_(Terminal_Sink, config);
}

ISink* BackendDispatcher::open_sink(const address::IoURI& uri,
                                    const char* force_format,
                                    const Config& config) {
    if (!uri.is_valid()) {
        roc_panic("backend dispatcher: invalid uri");
    }

    const DriverType driver_type = select_driver_type(uri);
    const char* driver_name = select_driver_name(uri, force_format);

    return (ISink*)open_terminal_(Terminal_Sink, driver_type, driver_name, uri.path(),
                                  config);
}

ISource* BackendDispatcher::open_source(const address::IoURI& uri,
                                        const char* force_format,
                                        const Config& config) {
    if (!uri.is_valid()) {
        roc_panic("backend dispatcher: invalid uri");
    }

    const DriverType driver_type = select_driver_type(uri);
    const char* driver_name = select_driver_name(uri, force_format);

    return (ISource*)open_terminal_(Terminal_Source, driver_type, driver_name, uri.path(),
                                    config);
}

bool BackendDispatcher::get_supported_schemes(core::StringList& list) {
    list.clear();

    for (size_t n = 0; n < drivers_.size(); n++) {
        // every device driver has its own scheme
        if (drivers_[n].type == DriverType_Device) {
            if (!list.push_back_unique(drivers_[n].name)) {
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

    for (size_t n = 0; n < drivers_.size(); n++) {
        if (drivers_[n].type == DriverType_File) {
            if (!list.push_back_unique(drivers_[n].name)) {
                return false;
            }
        }
    }

    return true;
}

ITerminal* BackendDispatcher::open_default_terminal_(TerminalType terminal_type,
                                                     const Config& config) {
    const unsigned driver_flags = DriverFlag_IsDefault
        | (terminal_type == Terminal_Sink ? DriverFlag_SupportsSink
                                          : DriverFlag_SupportsSource);

    for (size_t n = 0; n < drivers_.size(); n++) {
        if (!match_driver(drivers_[n], NULL, DriverType_Device, driver_flags)) {
            continue;
        }

        ITerminal* terminal = drivers_[n].backend->open_terminal(
            allocator_, terminal_type, DriverType_Device, drivers_[n].name, "default",
            config);
        if (terminal) {
            return terminal;
        }
    }

    roc_log(LogError, "backend dispatcher: failed to open default terminal");
    return NULL;
}

ITerminal* BackendDispatcher::open_terminal_(TerminalType terminal_type,
                                             DriverType driver_type,
                                             const char* driver_name,
                                             const char* path,
                                             const Config& config) {
    const unsigned driver_flags =
        (terminal_type == Terminal_Sink ? DriverFlag_SupportsSink
                                        : DriverFlag_SupportsSource);

    if (driver_name != NULL) {
        for (size_t n = 0; n < drivers_.size(); n++) {
            if (!match_driver(drivers_[n], driver_name, driver_type, driver_flags)) {
                continue;
            }

            ITerminal* terminal = drivers_[n].backend->open_terminal(
                allocator_, terminal_type, driver_type, driver_name, path, config);
            if (terminal) {
                return terminal;
            }
        }
    } else {
        for (size_t n = 0; n < n_backends_; n++) {
            ITerminal* terminal = backends_[n]->open_terminal(
                allocator_, terminal_type, driver_type, NULL, path, config);
            if (terminal) {
                return terminal;
            }
        }
    }

    roc_log(LogError, "backend dispatcher: failed to open %s: type=%s driver=%s path=%s",
            terminal_type_to_str(terminal_type), driver_type_to_str(driver_type),
            driver_name, path);

    return NULL;
}

void BackendDispatcher::register_backends_() {
#ifdef ROC_TARGET_PULSEAUDIO
    register_backend_(PulseaudioBackend::instance());
#endif // ROC_TARGET_PULSEAUDIO
#ifdef ROC_TARGET_SOX
    register_backend_(SoxBackend::instance());
#endif // ROC_TARGET_SOX
}

void BackendDispatcher::register_backend_(IBackend& backend) {
    roc_panic_if(n_backends_ == MaxBackends);
    backends_[n_backends_++] = &backend;
}

void BackendDispatcher::discover_drivers_() {
    if (!drivers_.grow(MaxDrivers)) {
        roc_panic("backend dispatcher: allocation failed");
    }

    for (size_t n = 0; n < n_backends_; n++) {
        backends_[n]->get_drivers(drivers_);
    }

    roc_log(LogDebug, "backend dispatcher: initialized: n_backends=%d n_drivers=%d",
            (int)n_backends_, (int)drivers_.size());
}

} // namespace sndio
} // namespace roc
