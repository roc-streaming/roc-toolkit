/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/driver_info.h
//! @brief Driver info interface.

#ifndef ROC_SNDIO_DRIVER_INFO_H_
#define ROC_SNDIO_DRIVER_INFO_H_

#include "roc_core/array.h"

namespace roc {
namespace sndio {

//! Driver info interface.
struct DriverInfo {
    DriverInfo();

    //! Max size of string
    enum { MaxSize = 20 };

    //! Parameterized Constructor initializes name, assumes driver_name is terminated with
    //! null char
    explicit DriverInfo(const char* driver_name);

    //! Placeholder for the driver name
    char name[MaxSize];
};

//! Append driver to array and ensure uniqueness, returns false if unable to allocate
//! space for item.
bool add_driver_uniq(core::Array<DriverInfo>& arr, const char* driver_name);

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_DRIVER_INFO_H_
