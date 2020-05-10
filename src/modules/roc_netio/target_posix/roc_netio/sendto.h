/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_posix/roc_netio/sendto.h
//! @brief Wrapper for non-blocking sendto() syscall.

#ifndef ROC_NETIO_SENDTO_H_
#define ROC_NETIO_SENDTO_H_

#include "roc_address/socket_addr.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace netio {

//! Try to send a non-blocking POSIX sendto().
//! Returns true if successful.
//!
//! @b Parameters
//!  - @p fd file descriptor of the socket
//!  - @p buf pointer to the buffer with the message to send
//!  - @p bufsz size of the buffer
//!  - @p dst_addr destination socket
bool sendto_nb(int fd,
               const void* buf,
               size_t bufsz,
               const address::SocketAddr& dst_addr);

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_SENDTO_H_
