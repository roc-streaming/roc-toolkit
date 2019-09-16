/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/log.h"
#include "roc_packet/address_to_str.h"

namespace roc {
namespace packet {

address_to_str::address_to_str(const Address& addr) {
    buffer_[0] = '\0';

    switch (addr.version()) {
    case 4: {
        if (!addr.get_host(buffer_, sizeof(buffer_))) {
            roc_log(LogError, "address to str: can't format ip");
            strcpy(buffer_, "<error>");

            return;
        }

        size_t blen = strlen(buffer_);

        if (snprintf(buffer_ + blen, sizeof(buffer_) - blen, ":%d", addr.port()) < 0) {
            roc_log(LogError, "address to str: can't format port");
            strcpy(buffer_, "<error>");

            return;
        }

        if (!addr.has_miface()) {
            return;
        }

        blen = strlen(buffer_);

        if (snprintf(buffer_ + blen, sizeof(buffer_) - blen, " miface ") < 0) {
            roc_log(LogError, "address to str: can't format miface");
            strcpy(buffer_, "<error>");

            return;
        }

        blen = strlen(buffer_);

        if (!addr.get_miface(buffer_ + blen, sizeof(buffer_) - blen)) {
            roc_log(LogError, "address to str: can't format miface");
            strcpy(buffer_, "<error>");

            return;
        }

        break;
    }
    case 6: {
        buffer_[0] = '[';

        if (!addr.get_host(buffer_ + 1, sizeof(buffer_) - 1)) {
            roc_log(LogError, "address to str: can't format ip");
            strcpy(buffer_, "<error>");

            return;
        }

        size_t blen = strlen(buffer_);
        if (snprintf(buffer_ + blen, sizeof(buffer_) - blen, "]:%d", addr.port()) < 0) {
            roc_log(LogError, "address to str: can't format port");
            strcpy(buffer_, "<error>");

            return;
        }

        if (!addr.has_miface()) {
            return;
        }

        blen = strlen(buffer_);

        if (snprintf(buffer_ + blen, sizeof(buffer_) - blen, " miface [") < 0) {
            roc_log(LogError, "address to str: can't format miface");
            strcpy(buffer_, "<error>");

            return;
        }

        blen = strlen(buffer_);

        if (!addr.get_miface(buffer_ + blen, sizeof(buffer_) - blen)) {
            roc_log(LogError, "address to str: can't format miface");
            strcpy(buffer_, "<error>");

            return;
        }

        blen = strlen(buffer_);

        if (snprintf(buffer_ + blen, sizeof(buffer_) - blen, "]") < 0) {
            roc_log(LogError, "address to str: can't format miface");
            strcpy(buffer_, "<error>");

            return;
        }

        break;
    }
    default:
        strcpy(buffer_, "<none>");
        break;
    }
}

} // namespace packet
} // namespace roc
