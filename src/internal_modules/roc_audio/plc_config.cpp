/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/plc_config.h"

namespace roc {
namespace audio {

bool PlcConfig::deduce_defaults() {
    if (backend == PlcBackend_Default) {
        backend = PlcBackend_None;
    }

    return true;
}

const char* plc_backend_to_str(PlcBackend backend) {
    switch (backend) {
    case PlcBackend_Default:
        return "default";
    case PlcBackend_None:
        return "none";
    case PlcBackend_Beep:
        return "beep";
    case PlcBackend_Max:
        break;
    }

    return "unknown";
}

} // namespace audio
} // namespace roc
