/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/log.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/print_drivers.h"

namespace roc {
namespace sndio {

namespace {

enum { ArraySize = 100, LineSize = 70 };

void print_driver_names(const core::Array<DriverInfo>& arr,
                        const char* prefix,
                        const char* suffix) {
    for (size_t n = 0; n < arr.size(); n++) {
        printf(" ");

        int size = 0;
        while (size < LineSize && n < arr.size()) {
            int ret = printf(" %s%s%s", prefix, arr[n++].name, suffix);
            if (ret > 0) {
                size += ret;
            }
        }

        printf("\n");
    }
}

} // namespace

bool print_drivers(core::IAllocator& allocator) {
    core::Array<DriverInfo> arr(allocator);

    if (!arr.grow(ArraySize)) {
        roc_log(LogError, "can't preallocate array");
        return false;
    }

    if (!BackendDispatcher::instance().get_device_drivers(arr)) {
        roc_log(LogError, "can't retrieve driver list");
        return false;
    }

    printf("supported device drivers:\n");
    print_driver_names(arr, "", "");

    if (!BackendDispatcher::instance().get_file_drivers(arr)) {
        roc_log(LogError, "can't retrieve format list");
        return false;
    }

    printf("\nsupported file drivers:\n");
    print_driver_names(arr, "", "");

    return true;
}

} // namespace sndio
} // namespace roc
