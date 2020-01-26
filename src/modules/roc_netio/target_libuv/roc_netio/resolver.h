/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/resolver.h
//! @brief Hostname resolver.

#ifndef ROC_NETIO_RESOLVER_H_
#define ROC_NETIO_RESOLVER_H_

#include <uv.h>

#include "roc_core/noncopyable.h"
#include "roc_netio/resolver_request.h"
#include "roc_netio/resolver_request_handler.h"

namespace roc {
namespace netio {

//! Hostname resolver.
class Resolver : public core::NonCopyable<> {
public:
    //! Initialize.
    Resolver(IResolverRequestHandler& req_handler, uv_loop_t& event_loop);

    //! Initiate asynchronous resolve request.
    //!
    //! Should be called from event loop thread.
    //! Resolving itself will be run on the internal libuv thread pool.
    //!
    //! When resolving is finished, IRequestHandler::handle_resolved() will be
    //! called on the event loop thread.
    //!
    //! If there is no need for resolving or asynchronous request can't be started,
    //! fills @p req and returns false.
    bool async_resolve(ResolverRequest& req);

private:
    static void getaddrinfo_cb_(uv_getaddrinfo_t* req, int status, struct addrinfo* res);

    void finish_resolving_(ResolverRequest& req, int status);

    uv_loop_t& loop_;

    IResolverRequestHandler& req_handler_;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_RESOLVER_H_
