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
#include "roc_status/code_to_str.h"

namespace roc {
namespace sndio {

namespace {

bool match_driver(const DriverInfo& driver_info,
                  unsigned driver_flags,
                  const char* driver_name) {
    if (driver_name && strcmp(driver_info.driver_name, driver_name) != 0) {
        return false;
    }

    if ((driver_info.driver_flags & driver_flags) != driver_flags) {
        return false;
    }

    return true;
}

bool match_format(const FormatInfo& format_info,
                  unsigned driver_flags,
                  const char* format_name) {
    if (format_name && strcmp(format_info.format_name, format_name) != 0) {
        return false;
    }

    if ((format_info.driver_flags & driver_flags) != driver_flags) {
        return false;
    }

    return true;
}

} // namespace

BackendDispatcher::BackendDispatcher(core::IPool& frame_pool,
                                     core::IPool& buffer_pool,
                                     core::IArena& arena)
    : frame_factory_(frame_pool, buffer_pool)
    , arena_(arena) {
}

status::StatusCode BackendDispatcher::open_default_sink(const IoConfig& io_config,
                                                        core::ScopedPtr<ISink>& result) {
    IDevice* device = NULL;

    const status::StatusCode code =
        open_default_device_(DeviceType_Sink, io_config, &device);
    if (code != status::StatusOK) {
        return code;
    }

    roc_panic_if_msg(!device || device->type() != DeviceType_Sink || !device->to_sink(),
                     "backend dispatcher: unexpected device");

    result.reset(device->to_sink());
    return status::StatusOK;
}

status::StatusCode
BackendDispatcher::open_default_source(const IoConfig& io_config,
                                       core::ScopedPtr<ISource>& result) {
    IDevice* device = NULL;

    const status::StatusCode code =
        open_default_device_(DeviceType_Source, io_config, &device);
    if (code != status::StatusOK) {
        return code;
    }

    roc_panic_if_msg(!device || device->type() != DeviceType_Source
                         || !device->to_source(),
                     "backend dispatcher: unexpected device");

    result.reset(device->to_source());
    return status::StatusOK;
}

status::StatusCode BackendDispatcher::open_sink(const address::IoUri& uri,
                                                const IoConfig& io_config,
                                                core::ScopedPtr<ISink>& result) {
    if (!uri.is_valid()) {
        roc_panic("backend dispatcher: invalid uri");
    }

    const char* driver = uri.scheme();
    const char* path = uri.path();

    IDevice* device = NULL;

    const status::StatusCode code =
        open_file_or_device_(DeviceType_Sink, driver, path, io_config, &device);
    if (code != status::StatusOK) {
        return code;
    }

    roc_panic_if_msg(!device || device->type() != DeviceType_Sink || !device->to_sink(),
                     "backend dispatcher: unexpected device");

    result.reset(device->to_sink());
    return status::StatusOK;
}

status::StatusCode BackendDispatcher::open_source(const address::IoUri& uri,
                                                  const IoConfig& io_config,
                                                  core::ScopedPtr<ISource>& result) {
    if (!uri.is_valid()) {
        roc_panic("backend dispatcher: invalid uri");
    }

    const char* driver = uri.scheme();
    const char* path = uri.path();

    IDevice* device = NULL;

    const status::StatusCode code =
        open_file_or_device_(DeviceType_Source, driver, path, io_config, &device);
    if (code != status::StatusOK) {
        return code;
    }

    roc_panic_if_msg(!device || device->type() != DeviceType_Source
                         || !device->to_source(),
                     "backend dispatcher: unexpected device");

    result.reset(device->to_source());
    return status::StatusOK;
}

bool BackendDispatcher::get_supported_schemes(core::StringList& result) {
    result.clear();

    for (size_t n = 0; n < BackendMap::instance().num_drivers(); n++) {
        const DriverInfo& driver_info = BackendMap::instance().nth_driver(n);

        if (!result.find(driver_info.driver_name)) {
            if (!result.push_back(driver_info.driver_name)) {
                return false;
            }
        }
    }

    return true;
}

bool BackendDispatcher::get_supported_formats(core::StringList& result) {
    result.clear();

    for (size_t n = 0; n < BackendMap::instance().num_formats(); n++) {
        const FormatInfo& format_info = BackendMap::instance().nth_format(n);

        if (!result.find(format_info.format_name)) {
            if (!result.push_back(format_info.format_name)) {
                return false;
            }
        }
    }

    return true;
}

bool BackendDispatcher::get_supported_subformat_groups(core::StringList& result) {
    result.clear();

    for (size_t n = 0; n < BackendMap::instance().num_backends(); n++) {
        IBackend& backend = BackendMap::instance().nth_backend(n);

        if (!backend.discover_subformat_groups(result)) {
            return false;
        }
    }

    return true;
}

bool BackendDispatcher::get_supported_subformats(const char* group,
                                                 core::StringList& result) {
    result.clear();

    for (size_t n = 0; n < BackendMap::instance().num_backends(); n++) {
        IBackend& backend = BackendMap::instance().nth_backend(n);

        if (!backend.discover_subformats(group, result)) {
            return false;
        }
    }

    return true;
}

status::StatusCode BackendDispatcher::open_default_device_(DeviceType device_type,
                                                           const IoConfig& io_config,
                                                           IDevice** result) {
    roc_panic_if(!result);

    status::StatusCode code = status::StatusNoDriver;

    // Try all drivers with Driver_DefaultDevice flag.
    const unsigned driver_flags = Driver_Device | Driver_DefaultDevice
        | (device_type == DeviceType_Sink ? Driver_SupportsSink : Driver_SupportsSource);

    for (size_t n = 0; n < BackendMap::instance().num_drivers(); n++) {
        const DriverInfo& driver_info = BackendMap::instance().nth_driver(n);

        if (!match_driver(driver_info, driver_flags, NULL)) {
            continue;
        }

        code = driver_info.backend->open_device(device_type, driver_info.driver_name,
                                                "default", io_config, frame_factory_,
                                                arena_, result);

        if (code == status::StatusOK) {
            return code;
        }

        if (code == status::StatusNoDriver) {
            continue;
        }

        break;
    }

    roc_log(LogError, "backend dispatcher: failed to open default device: status=%s",
            status::code_to_str(code));

    return code;
}

status::StatusCode BackendDispatcher::open_file_or_device_(DeviceType device_type,
                                                           const char* driver,
                                                           const char* path,
                                                           const IoConfig& io_config,
                                                           IDevice** result) {
    roc_panic_if(!driver);
    roc_panic_if(!path);
    roc_panic_if(!result);

    if (strcmp(driver, "file") == 0) {
        if (io_config.latency != 0) {
            roc_log(LogError,
                    "backend dispatcher: it's not possible to specify io latency"
                    " for files");
            return status::StatusBadConfig;
        }

        if (device_type == DeviceType_Sink && strcmp(path, "-") == 0
            && !io_config.sample_spec.has_format()) {
            roc_log(
                LogError,
                "backend dispatcher: when output file is \"-\", format must be specified"
                " explicitly via io encoding");
            return status::StatusBadConfig;
        }

        return open_file_(device_type, driver, path, io_config, result);
    }

    return open_device_(device_type, driver, path, io_config, result);
}

status::StatusCode BackendDispatcher::open_device_(DeviceType device_type,
                                                   const char* driver,
                                                   const char* path,
                                                   const IoConfig& io_config,
                                                   IDevice** result) {
    status::StatusCode code = status::StatusNoDriver;

    const unsigned driver_flags = Driver_Device
        | (device_type == DeviceType_Sink ? Driver_SupportsSink : Driver_SupportsSource);

    // We're opening device, driver defines device type (pulseaudio, alsa, etc).
    // Try backends which support matching driver.
    for (size_t n = 0; n < BackendMap::instance().num_drivers(); n++) {
        const DriverInfo& driver_info = BackendMap::instance().nth_driver(n);

        if (!match_driver(driver_info, driver_flags, driver)) {
            continue;
        }

        code = driver_info.backend->open_device(device_type, driver, path, io_config,
                                                frame_factory_, arena_, result);

        if (code == status::StatusOK) {
            return code;
        }

        if (code == status::StatusNoDriver) {
            // No error, backend just doesn't support driver.
            continue;
        }

        break;
    }

    roc_log(LogError,
            "backend dispatcher: failed to open device:"
            " device_type=%s driver=%s path=%s status=%s",
            device_type_to_str(device_type), driver, path, status::code_to_str(code));

    return code;
}

status::StatusCode BackendDispatcher::open_file_(DeviceType device_type,
                                                 const char* driver,
                                                 const char* path,
                                                 const IoConfig& io_config,
                                                 IDevice** result) {
    status::StatusCode code = status::StatusNoDriver;

    const unsigned driver_flags = Driver_File
        | (device_type == DeviceType_Sink ? Driver_SupportsSink : Driver_SupportsSource);

    if (io_config.sample_spec.has_format()) {
        // We're opening file and format is specified explicitly (wav, flac, etc).
        // Try backends which support requested format.
        for (size_t n = 0; n < BackendMap::instance().num_formats(); n++) {
            const FormatInfo& format_info = BackendMap::instance().nth_format(n);

            if (!match_format(format_info, driver_flags,
                              io_config.sample_spec.format_name())) {
                continue;
            }

            code = format_info.backend->open_device(device_type, driver, path, io_config,
                                                    frame_factory_, arena_, result);

            if (code == status::StatusOK) {
                return code;
            }

            if (code == status::StatusNoDriver || code == status::StatusNoFormat) {
                // No error, backend just doesn't support driver or format.
                continue;
            }

            break;
        }
    } else {
        // We're opening file and format is omitted.
        // Try all backends.
        for (size_t n = 0; n < BackendMap::instance().num_backends(); n++) {
            IBackend& backend = BackendMap::instance().nth_backend(n);

            code = backend.open_device(device_type, driver, path, io_config,
                                       frame_factory_, arena_, result);

            if (code == status::StatusOK) {
                return code;
            }

            if (code == status::StatusNoDriver || code == status::StatusNoFormat) {
                // No error, backend just doesn't support driver or format.
                continue;
            }

            break;
        }
    }

    roc_log(LogError,
            "backend dispatcher: failed to open file:"
            " device_type=%s driver=%s path=%s status=%s",
            device_type_to_str(device_type), driver, path, status::code_to_str(code));

    return code;
}

} // namespace sndio
} // namespace roc
