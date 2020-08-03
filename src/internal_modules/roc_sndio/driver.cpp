/*
 * Copyright (c) 2022 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/driver.h"

namespace roc {
namespace sndio {

const char* driver_type_to_str(DriverType type) {
    switch (type) {
    case DriverType_Device:
        return "device";

    case DriverType_File:
        return "file";

    case DriverType_Invalid:
    default:
        break;
    }

    return "<invalid>";
}

} // namespace sndio
} // namespace roc
