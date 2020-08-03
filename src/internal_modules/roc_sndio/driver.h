/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/driver.h
//! @brief Driver types.

#ifndef ROC_SNDIO_DRIVER_H_
#define ROC_SNDIO_DRIVER_H_

#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace sndio {

class IBackend;

//! Driver type.
enum DriverType {
    //! Invalid type.
    DriverType_Invalid,

    //! Driver for audio files.
    DriverType_File,

    //! Driver for audio devices.
    DriverType_Device
};

//! Driver flags.
enum DriverFlags {
    //! Driver is used if no file or device is specified.
    DriverFlag_IsDefault = (1 << 0),

    //! Driver supports sources (input).
    DriverFlag_SupportsSource = (1 << 1),

    //! Driver supports sinks (output).
    DriverFlag_SupportsSink = (1 << 2)
};

//! Driver information.
struct DriverInfo {
    //! Driver name.
    char name[20];

    //! Driver type.
    DriverType type;

    //! Driver flags.
    unsigned int flags;

    //! Backend the driver uses.
    IBackend* backend;

    //! Initialize.
    DriverInfo()
        : type(DriverType_Invalid)
        , flags(0)
        , backend(NULL) {
        strcpy(name, "");
    }

    //! Initialize.
    DriverInfo(const char* driver_name,
               DriverType driver_type,
               unsigned int driver_flags,
               IBackend* driver_backend)
        : type(driver_type)
        , flags(driver_flags)
        , backend(driver_backend) {
        if (!driver_name || strlen(driver_name) > sizeof(name) - 1) {
            roc_panic("invalid driver name");
        }
        strcpy(name, driver_name);
    }
};

//! Convert driver type to string.
const char* driver_type_to_str(DriverType type);

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_DRIVER_H_
