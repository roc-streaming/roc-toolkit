/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cerrno>
#include <sys/socket.h>
#include <sys/types.h>

#include "roc_core/errno_to_str.h"
#include "roc_core/log.h"
#include "roc_netio/sendto.h"

namespace roc {
namespace netio {

bool sendto_nb(int fd,
               const void* buf,
               size_t bufsz,
               const address::SocketAddr& dst_addr) {
    if (sendto(fd, buf, bufsz, MSG_DONTWAIT, dst_addr.saddr(), dst_addr.slen()) < 0) {
        // The preprocessor condition is needed to avoid a logical-op warning (EAGAIN and
        // EWOULDBLOCK are equivalent in most systems)
        if (errno != EAGAIN
#if EAGAIN != EWOULDBLOCK
            && errno != EWOULDBLOCK
#endif
        ) {
            roc_log(LogError, "sendto_nb: sendto: %s", core::errno_to_str().c_str());
        }
        return false;
    }
    return true;
}

} // namespace netio
} // namespace roc
