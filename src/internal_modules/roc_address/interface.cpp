/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/interface.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace address {

const char* interface_to_str(Interface iface) {
    switch (iface) {
    case Iface_Invalid:
        break;

    case Iface_Aggregate:
        return "aggregate";

    case Iface_AudioSource:
        return "audiosrc";

    case Iface_AudioRepair:
        return "audiorpr";

    case Iface_AudioControl:
        return "audioctl";

    case Iface_Max:
        break;
    }

    return NULL;
}

} // namespace address
} // namespace roc
