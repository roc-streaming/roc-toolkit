/*
 * Copyright (c) 2019 Roc authors
 *
 * This Sink Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <string.h>

#include "roc_sndio/driver_info.h"

namespace roc {
namespace sndio {

DriverInfo::DriverInfo() {
    for (size_t x = 0; x < MaxSize; x++) {
        name[x] = '\0';
    }
}

DriverInfo::DriverInfo(const char* driver_name) {
    size_t length = strlen(driver_name);
    if (length > MaxSize - 1) {
        length = MaxSize - 1;
    }
    strncpy(name, driver_name, MaxSize);
    name[length] = '\0';
}

bool add_driver_uniq(core::Array<DriverInfo>& arr, const char* driver_name) {
    roc_panic_if(driver_name == NULL);
    for (size_t n = 0; n < arr.size(); n++) {
        if (strcmp(driver_name, arr[n].name) == 0) {
            return true;
        }
    }
    if (arr.grow(arr.size() + 1)) {
        DriverInfo new_driver(driver_name);
        arr.push_back(new_driver);
        return true;
    } else {
        return false;
    }
}

} // namespace sndio
} // namespace roc
