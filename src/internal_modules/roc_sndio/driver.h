/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/driver.h
//! @brief Driver information.

#ifndef ROC_SNDIO_DRIVER_H_
#define ROC_SNDIO_DRIVER_H_

#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace sndio {

class IBackend;

//! Maximum number of drivers.
static const size_t MaxDrivers = 16;

//! Maximum number of file formats.
static const size_t MaxFormats = 128;

//! Driver flags.
enum DriverFlags {
    //! This is driver for audio files.
    Driver_File = (1 << 0),

    //! This is driver for audio devices.
    Driver_Device = (1 << 1),

    //! Driver is used if no file or device is specified.
    Driver_DefaultDevice = (1 << 2),

    //! Driver supports sources (input).
    Driver_SupportsSource = (1 << 3),

    //! Driver supports sinks (output).
    Driver_SupportsSink = (1 << 4)
};

//! Information about driver.
struct DriverInfo {
    //! Driver name.
    char driver_name[12];

    //! Driver flags.
    unsigned int driver_flags;

    //! Associated backend.
    IBackend* backend;

    //! Initialize.
    DriverInfo() {
        strcpy(driver_name, "");
        driver_flags = 0;
        backend = NULL;
    }

    //! Initialize.
    DriverInfo(const char* p_driver_name,
               unsigned int p_driver_flags,
               IBackend* p_backend) {
        if (!p_driver_name || strlen(p_driver_name) > sizeof(driver_name) - 1) {
            roc_panic("invalid driver name");
        }
        strcpy(driver_name, p_driver_name);
        driver_flags = p_driver_flags;
        backend = p_backend;
    }
};

//! Information about format supported by "file" driver.
struct FormatInfo {
    //! Driver name.
    char driver_name[12];

    //! Driver flags.
    unsigned int driver_flags;

    //! Format name.
    char format_name[12];

    //! Associated backend.
    IBackend* backend;

    //! Initialize.
    FormatInfo() {
        strcpy(driver_name, "");
        driver_flags = 0;
        strcpy(format_name, "");
        backend = NULL;
    }

    //! Initialize.
    FormatInfo(const char* p_driver_name,
               const char* p_format_name,
               unsigned int p_driver_flags,
               IBackend* p_backend) {
        if (!p_format_name || strlen(p_format_name) > sizeof(format_name) - 1) {
            roc_panic("invalid format name");
        }
        strcpy(driver_name, p_driver_name);
        driver_flags = p_driver_flags;
        strcpy(format_name, p_format_name);
        backend = p_backend;
    }
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_DRIVER_H_
