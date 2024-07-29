/*
 * Copyright (c) 2022 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/device_state.h"

namespace roc {
namespace sndio {

const char* device_state_to_str(DeviceState state) {
    switch (state) {
    case DeviceState_Active:
        return "active";

    case DeviceState_Idle:
        return "idle";

    case DeviceState_Paused:
        return "paused";

    case DeviceState_Broken:
        return "broken";

    case DeviceState_Closed:
        return "closed";

    default:
        break;
    }

    return "<invalid>";
}

} // namespace sndio
} // namespace roc
