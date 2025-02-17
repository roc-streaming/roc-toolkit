/*
 * Copyright (c) 2022 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/device_type.h"

namespace roc {
namespace sndio {

const char* device_type_to_str(DeviceType type) {
    switch (type) {
    case DeviceType_Sink:
        return "sink";

    case DeviceType_Source:
        return "source";

    default:
        break;
    }

    return "<invalid>";
}

} // namespace sndio
} // namespace roc
