/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/driver.h
//! @brief TODO.

#ifndef ROC_SNDIO_DRIVER_H_
#define ROC_SNDIO_DRIVER_H_

namespace roc {
namespace sndio {

class IBackend;

//! Driver flags
enum DriverFlags {
    //! Default driver.
    DriverDefault = (1 << 0),
    //! File driver.
    DriverFile = (1 << 1),
    //! Device driver.
    DriverDevice = (1 << 2),
    //! Driver supports sources.
    DriverSource = (1 << 3),
    //! Driver supports sinks.
    DriverSink = (1 << 4)
};

//! Driver information.
typedef struct DriverInfo {
    //! Driver name.
    char name[20];

    //! Backend the driver uses.
    IBackend* backend;

    //! Driver Flags.
    unsigned int flags;

    //! Initialize.
    DriverInfo()
        : backend(NULL)
        , flags(0) {
        strcpy(name, "");
    }
    //! Setter method.
    void set(const char* driver, IBackend* d_backend, unsigned int d_flags) {
        strcpy(name, driver);
        backend = d_backend;
        flags = d_flags;
    }
} DriverInfo;

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_DRIVER_H_
