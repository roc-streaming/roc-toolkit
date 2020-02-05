/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/addr_family.h"

namespace roc {
namespace address {

const char* addr_family_to_str(AddrFamily family) {
    switch ((int)family) {
    case Family_IPv4:
        return "IPv4";

    case Family_IPv6:
        return "IPv6";

    default:
        break;
    }

    return "unknown";
}

} // namespace address
} // namespace roc
