/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/DriverInfo.h
//! @brief Driver info interface.

#ifndef ROC_SNDIO_DRIVERINFO_H_
#define ROC_SNDIO_DRIVERINFO_H_

namespace roc {
namespace sndio {

//! Driver info interface.
class DriverInfo {
public:
    DriverInfo();
    DriverInfo(const char* name)
        : name(name){};
    const char* name;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_DRIVERINFO_H_
