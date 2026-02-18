/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/resolver_request.h
//! @brief Resolver request.

#ifndef ROC_NETIO_RESOLVER_REQUEST_H_
#define ROC_NETIO_RESOLVER_REQUEST_H_

#include <uv.h>

#include "roc_address/network_uri.h"
#include "roc_address/socket_addr.h"

namespace roc {
namespace netio {

//! Resolver request.
struct ResolverRequest {
    //! Endpoint with hostname to resolve for async_resolve().
    const address::NetworkUri* endpoint_uri;

    //! Resolved address to be filled by async_resolve().
    address::SocketAddr resolved_address;

    //! Successfully resolved.
    bool success;

    //! libuv request handle.
    uv_getaddrinfo_t handle;

    ResolverRequest()
        : endpoint_uri(NULL)
        , success(false) {
        memset(&handle, 0, sizeof(handle));
    }
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_RESOLVER_REQUEST_H_
