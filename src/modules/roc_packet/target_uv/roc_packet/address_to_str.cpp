/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <uv.h>

#include "roc_core/endian.h"
#include "roc_core/errno_to_str.h"
#include "roc_core/log.h"
#include "roc_packet/address_to_str.h"

namespace roc {
namespace packet {

address_to_str::address_to_str(const Address& addr) {
    buffer_[0] = '\0';

    switch (addr.saddr()->sa_family) {
    case AF_INET: {
        const sockaddr_in* sa = (const sockaddr_in*)addr.saddr();

        if (int err = uv_inet_ntop(AF_INET, &sa->sin_addr, buffer_, sizeof(buffer_))) {
            roc_log(LogError, "address to str: uv_inet_ntop(): [%s] %s", uv_err_name(err),
                    uv_strerror(err));
        }

        const size_t blen = strlen(buffer_);
        if (snprintf(buffer_ + blen, sizeof(buffer_) - blen, ":%u",
                     (unsigned)ROC_NTOH_16(sa->sin_port))
            < 0) {
            roc_log(LogError, "address to str: snprintf(): %s",
                    core::errno_to_str().c_str());
        }

        break;
    }
    case AF_INET6: {
        buffer_[0] = '[';

        const sockaddr_in6* sa = (const sockaddr_in6*)addr.saddr();

        if (int err = uv_inet_ntop(AF_INET6, &sa->sin6_addr, buffer_ + 1,
                                   sizeof(buffer_) - 1)) {
            roc_log(LogError, "address to str: uv_inet_ntop(): [%s] %s", uv_err_name(err),
                    uv_strerror(err));
        }

        const size_t blen = strlen(buffer_);
        if (snprintf(buffer_ + blen, sizeof(buffer_) - blen, "]:%u",
                     (unsigned)ROC_NTOH_16(sa->sin6_port))
            < 0) {
            roc_log(LogError, "address to str: snprintf(): %s",
                    core::errno_to_str().c_str());
        }

        break;
    }
    default:
        break;
    }
}

} // namespace packet
} // namespace roc
