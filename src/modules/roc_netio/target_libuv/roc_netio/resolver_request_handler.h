/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/resolver_request_handler.h
//! @brief Resolver request result handler.

#ifndef ROC_NETIO_RESOLVER_REQUEST_HANDLER_H_
#define ROC_NETIO_RESOLVER_REQUEST_HANDLER_H_

#include "roc_netio/resolver_request.h"

namespace roc {
namespace netio {

//! Resolver request result handler.
class IResolverRequestHandler {
public:
    virtual ~IResolverRequestHandler();

    //! Invoked on event loop thread when resolve request is finished.
    virtual void handle_resolved(ResolverRequest& req) = 0;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_RESOLVER_REQUEST_HANDLER_H_
