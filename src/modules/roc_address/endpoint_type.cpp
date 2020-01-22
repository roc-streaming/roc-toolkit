/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_type.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace address {

const char* endpoint_type_to_str(EndpointType type) {
    switch (type) {
    case EndType_Session:
        return "session";

    case EndType_AudioSource:
        return "source";

    case EndType_AudioRepair:
        return "repair";
    }

    return NULL;
}

} // namespace address
} // namespace roc
