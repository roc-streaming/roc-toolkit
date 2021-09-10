/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/socket_options.h
//! @brief Socket options.

#ifndef ROC_NETIO_SOCKET_OPTIONS_H_
#define ROC_NETIO_SOCKET_OPTIONS_H_

namespace roc {
namespace netio {

//! Socket options.
struct SocketOptions {
    //! Disable Nagle's algorithm.
    bool disable_nagle;

    SocketOptions()
        : disable_nagle(true) {
    }
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_SOCKET_OPTIONS_H_
