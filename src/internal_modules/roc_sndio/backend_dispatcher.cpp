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
#include "roc_sndio/backend_map.h"

namespace roc {
namespace sndio {

namespace {

DriverType select_driver_type(const address::IoUri& uri) {
    if (uri.is_file()) {
        return DriverType_File;
    } else {
        return DriverType_Device;
    }
}

const char* select_driver_name(const address::IoUri& uri, const char* force_format) {
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

BackendDispatcher::BackendDispatcher(core::IArena& arena)
    : arena_(arena) {
}

ISink* BackendDispatcher::open_default_sink(const Config& config) {
    return (ISink*)open_default_device_(DeviceType_Sink, config);
}

ISource* BackendDispatcher::open_default_source(const Config& config) {
    return (ISource*)open_default_device_(DeviceType_Sink, config);
}

ISink* BackendDispatcher::open_sink(const address::IoUri& uri,
                                    const char* force_format,
                                    const Config& config) {
    if (!uri.is_valid()) {
        roc_panic("backend dispatcher: invalid uri");
    }

    const DriverType driver_type = select_driver_type(uri);
    const char* driver_name = select_driver_name(uri, force_format);

    return (ISink*)open_device_(DeviceType_Sink, driver_type, driver_name, uri.path(),
                                config);
}

ISource* BackendDispatcher::open_source(const address::IoUri& uri,
                                        const char* force_format,
                                        const Config& config) {
    if (!uri.is_valid()) {
        roc_panic("backend dispatcher: invalid uri");
    }

    const DriverType driver_type = select_driver_type(uri);
    const char* driver_name = select_driver_name(uri, force_format);

    return (ISource*)open_device_(DeviceType_Source, driver_type, driver_name, uri.path(),
                                  config);
}

bool BackendDispatcher::get_supported_schemes(core::StringList& result) {
    result.clear();

    for (size_t n = 0; n < BackendMap::instance().num_drivers(); n++) {
        const DriverInfo& driver_info = BackendMap::instance().nth_driver(n);

        // every device driver has its own scheme
        if (driver_info.type == DriverType_Device) {
            if (!result.push_unique(driver_info.name)) {
                return false;
            }
        }
    }

    // all file drivers has a single "file" scheme
    if (!result.push_back("file")) {
        return false;
    }

    return true;
}

bool BackendDispatcher::get_supported_formats(core::StringList& result) {
    result.clear();

    for (size_t n = 0; n < BackendMap::instance().num_drivers(); n++) {
        const DriverInfo& driver_info = BackendMap::instance().nth_driver(n);

        if (driver_info.type == DriverType_File) {
            if (!result.push_unique(driver_info.name)) {
                return false;
            }
        }
    }

    return true;
}

IDevice* BackendDispatcher::open_default_device_(DeviceType device_type,
                                                 const Config& config) {
    const unsigned driver_flags =
        unsigned(DriverFlag_IsDefault
                 | (device_type == DeviceType_Sink ? DriverFlag_SupportsSink
                                                   : DriverFlag_SupportsSource));

    for (size_t n = 0; n < BackendMap::instance().num_drivers(); n++) {
        const DriverInfo& driver_info = BackendMap::instance().nth_driver(n);

        if (!match_driver(driver_info, NULL, DriverType_Device, driver_flags)) {
            continue;
        }

        IDevice* device = BackendMap::instance().nth_driver(n).backend->open_device(
            device_type, DriverType_Device, BackendMap::instance().nth_driver(n).name,
            "default", config, arena_);
        if (device) {
            return device;
        }
    }

    roc_log(LogError, "backend dispatcher: failed to open default device");
    return NULL;
}

IDevice* BackendDispatcher::open_device_(DeviceType device_type,
                                         DriverType driver_type,
                                         const char* driver_name,
                                         const char* path,
                                         const Config& config) {
    const unsigned driver_flags =
        (device_type == DeviceType_Sink ? DriverFlag_SupportsSink
                                        : DriverFlag_SupportsSource);

    if (driver_name != NULL) {
        for (size_t n = 0; n < BackendMap::instance().num_drivers(); n++) {
            const DriverInfo& driver_info = BackendMap::instance().nth_driver(n);

            if (!match_driver(driver_info, driver_name, driver_type, driver_flags)) {
                continue;
            }

            IDevice* device = BackendMap::instance().nth_driver(n).backend->open_device(
                device_type, driver_type, driver_name, path, config, arena_);
            if (device) {
                return device;
            }
        }
    } else {
        for (size_t n = 0; n < BackendMap::instance().num_backends(); n++) {
            IBackend& backend = BackendMap::instance().nth_backend(n);

            IDevice* device =
                backend.open_device(device_type, driver_type, NULL, path, config, arena_);
            if (device) {
                return device;
            }
        }
    }

    roc_log(LogError, "backend dispatcher: failed to open %s: type=%s driver=%s path=%s",
            device_type_to_str(device_type), driver_type_to_str(driver_type), driver_name,
            path);

    return NULL;
}

} // namespace sndio
} // namespace roc
