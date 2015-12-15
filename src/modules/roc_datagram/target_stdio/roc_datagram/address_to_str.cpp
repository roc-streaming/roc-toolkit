/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_datagram/address_to_str.h"

namespace roc {
namespace datagram {

address_to_str::address_to_str(const Address& addr) {
    int sz = snprintf(buffer, sizeof(buffer) - 1, "%d.%d.%d.%d:%d",
                      (int)addr.ip[0], //
                      (int)addr.ip[1], //
                      (int)addr.ip[2], //
                      (int)addr.ip[3], //
                      (int)addr.port);

    if (sz < 0) {
        sz = 0;
    }

    buffer[sz] = '\0'; // Be portable?
}

} // namespace datagram
} // namespace roc
