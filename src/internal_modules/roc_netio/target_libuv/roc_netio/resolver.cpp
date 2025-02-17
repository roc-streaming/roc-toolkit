/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/resolver.h"
#include "roc_address/network_uri_to_str.h"
#include "roc_address/parse_socket_addr.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/panic.h"

namespace roc {
namespace netio {

Resolver::Resolver(IResolverRequestHandler& req_handler, uv_loop_t& event_loop)
    : loop_(event_loop)
    , req_handler_(req_handler) {
}

bool Resolver::async_resolve(ResolverRequest& req) {
    roc_panic_if(!req.endpoint_uri);

    req.resolved_address.clear();

    if (!req.endpoint_uri->verify(address::NetworkUri::Subset_Full)) {
        roc_log(LogError, "resolver: invalid endpoint");
        req.success = false;
        return false;
    }

    roc_log(LogTrace, "resolver: starting resolving: endpoint=%s",
            address::network_uri_to_str(*req.endpoint_uri).c_str());

    if (address::parse_socket_addr(req.endpoint_uri->host(), req.endpoint_uri->port(),
                                   req.resolved_address)) {
        finish_resolving_(req, 0);
        return false;
    }

    req.handle.data = this;

    if (int err =
            uv_getaddrinfo(&loop_, &req.handle, &Resolver::getaddrinfo_cb_,
                           req.endpoint_uri->host(), req.endpoint_uri->service(), NULL)) {
        finish_resolving_(req, err);
        return false;
    }

    return true;
}

void Resolver::getaddrinfo_cb_(uv_getaddrinfo_t* req_handle,
                               int status,
                               struct addrinfo* addrinfo) {
    roc_panic_if(!req_handle);
    ResolverRequest& req = *ROC_CONTAINER_OF(req_handle, ResolverRequest, handle);

    roc_panic_if(!req_handle->data);
    Resolver& self = *(Resolver*)req_handle->data;

    if (status == 0) {
        for (struct addrinfo* ai = addrinfo; ai; ai = ai->ai_next) {
            if (req.resolved_address.set_host_port_saddr(ai->ai_addr)) {
                break;
            }
        }
    }

    uv_freeaddrinfo(addrinfo);

    self.finish_resolving_(req, status);
    self.req_handler_.handle_resolved(req);
}

void Resolver::finish_resolving_(ResolverRequest& req, int status) {
    if (status != 0) {
        roc_log(LogError, "resolver: can't resolve hostname '%s': [%s] %s",
                req.endpoint_uri->host(), uv_err_name(status), uv_strerror(status));
        req.success = false;
        return;
    }

    if (!req.resolved_address.has_host_port()) {
        roc_log(LogError, "resolver: no address associated with hostname: hostname=%s",
                req.endpoint_uri->host());
        req.success = false;
        return;
    }

    req.success = true;
}

} // namespace netio
} // namespace roc
