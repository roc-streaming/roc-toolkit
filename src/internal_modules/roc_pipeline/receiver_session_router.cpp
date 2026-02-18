/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_session_router.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_rtcp/cname.h"
#include "roc_status/status_code.h"

namespace roc {
namespace pipeline {

ReceiverSessionRouter::ReceiverSessionRouter(core::IArena& arena)
    : route_pool_("route_pool", arena)
    , source_node_pool_("source_node_pool", arena)
    , source_route_map_(arena)
    , address_route_map_(arena)
    , cname_route_map_(arena)
    , session_route_map_(arena) {
}

ReceiverSessionRouter::~ReceiverSessionRouter() {
    remove_all_routes_();
}

size_t ReceiverSessionRouter::num_routes() {
    return route_list_.size();
}

core::SharedPtr<ReceiverSession>
ReceiverSessionRouter::find_by_source(packet::stream_source_t source_id) {
    SourceNode* node = source_route_map_.find(source_id);
    if (!node) {
        return NULL;
    }

    return node->route().session;
}

core::SharedPtr<ReceiverSession>
ReceiverSessionRouter::find_by_address(const address::SocketAddr& source_addr) {
    roc_panic_if(!source_addr);

    AddressNode* node = address_route_map_.find(source_addr);
    if (!node) {
        return NULL;
    }

    return node->route().session;
}

bool ReceiverSessionRouter::has_session(const core::SharedPtr<ReceiverSession>& session) {
    roc_panic_if(!session);

    return session_route_map_.find(session) != NULL;
}

status::StatusCode
ReceiverSessionRouter::add_session(const core::SharedPtr<ReceiverSession>& session,
                                   packet::stream_source_t source_id,
                                   const address::SocketAddr& source_addr) {
    roc_panic_if(!session);

    // Session and address should be unique, forbid registering same
    // session or address twice.
    if (source_addr && address_route_map_.find(source_addr)) {
        roc_log(LogError,
                "session router: conflict:"
                " another session already exists for source address %s",
                address::socket_addr_to_str(source_addr).c_str());
        return status::StatusNoRoute;
    }

    if (session_route_map_.find(session)) {
        roc_log(LogError, "session router: conflict: session already registered");
        return status::StatusNoRoute;
    }

    if (SourceNode* node = source_route_map_.find(source_id)) {
        Route& route = node->route();

        if (!route.source_addr && !route.session) {
            // SSRC exists, but its route does not have session and address.
            // In this case we update existing route and attach session
            // and address to it.
            roc_log(
                LogDebug,
                "session router:"
                " linking existing SSRC to a new session: ssrc=%lu cname=%s address=%s",
                (unsigned long)source_id, rtcp::cname_to_str(route.cname).c_str(),
                address::socket_addr_to_str(source_addr).c_str());

            if (source_addr) {
                route.source_addr = source_addr;

                if (!address_route_map_.insert(route.address_node)) {
                    roc_log(LogError, "session router: allocation failed");
                    remove_route_(&route);
                    return status::StatusNoMem;
                }
            }

            route.session = session;

            if (!session_route_map_.insert(route.session_node)) {
                roc_log(LogError, "session router: allocation failed");
                remove_route_(&route);
                return status::StatusNoMem;
            }

            return status::StatusOK;
        }

        // SSRC exists, and it already has a session and address.
        // In this case we first unlink SSRC from old route and proceed to
        // creating a new route.
        unlink_source(source_id);
    }

    // No route exist, create a new one.
    roc_log(LogDebug,
            "session router:"
            " SSRC does not exist, creating new route: ssrc=%lu address=%s",
            (unsigned long)source_id, address::socket_addr_to_str(source_addr).c_str());

    return create_route_(source_id, source_addr, NULL, session);
}

void ReceiverSessionRouter::remove_session(
    const core::SharedPtr<ReceiverSession>& session) {
    roc_panic_if(!session);

    SessionNode* node = session_route_map_.find(session);
    if (!node) {
        // Nothing to remove.
        roc_log(LogTrace,
                "session router:"
                " session does not exist, nothing to remove");

        return;
    }

    remove_route_(&node->route());
}

status::StatusCode ReceiverSessionRouter::link_source(packet::stream_source_t source_id,
                                                      const char* cname) {
    roc_panic_if(!cname || !*cname);
    roc_panic_if(strlen(cname) > rtcp::MaxCnameLen);

    // Find routes for SSRC and CNAME.
    core::SharedPtr<Route> source_route, cname_route;

    if (SourceNode* node = source_route_map_.find(source_id)) {
        source_route = &node->route();
    }

    if (CnameNode* node = cname_route_map_.find(cname)) {
        cname_route = &node->route();
    }

    // No routes exist for both SSRC and CNAME.
    if (!source_route && !cname_route) {
        // Create new route.
        roc_log(LogDebug,
                "session router: SSRC and CNAME don't exists, creating new route:"
                " ssrc=%lu cname=%s",
                (unsigned long)source_id, rtcp::cname_to_str(cname).c_str());

        return create_route_(source_id, address::SocketAddr(), cname, NULL);
    }

    // Routes exist for both SSRC and CNAME.
    if (source_route && cname_route) {
        if (source_route == cname_route) {
            // SSRC and CNAME are already linked, nothing to do.
            roc_log(LogTrace,
                    "session router: SSRC and CNAME already exist and linked:"
                    " ssrc=%lu cname=%s",
                    (unsigned long)source_id, rtcp::cname_to_str(cname).c_str());

            return status::StatusOK;
        } else {
            // Relink SSRC to new CNAME.
            roc_log(LogDebug,
                    "session router:"
                    " relinking existing SSRC to another existing CNAME:"
                    " ssrc=%lu old_cname=%s new_cname=%s",
                    (unsigned long)source_id,
                    rtcp::cname_to_str(source_route->cname).c_str(),
                    rtcp::cname_to_str(cname).c_str());

            return relink_source_(source_id, cname);
        }
    }

    // Only SSRC route exists, and it's already linked to different CNAME.
    if (source_route && source_route->cname[0]) {
        // Relink SSRC to new CNAME.
        roc_log(LogDebug,
                "session router: relinking existing SSRC to new CNAME:"
                " ssrc=%lu old_cname=%s new_cname=%s",
                (unsigned long)source_id, rtcp::cname_to_str(source_route->cname).c_str(),
                rtcp::cname_to_str(cname).c_str());

        return relink_source_(source_id, cname);
    }

    // Only SSRC route exists, and it's not linked to any CNAME.
    if (source_route && !source_route->cname[0]) {
        // Link CNAME to existing route.
        roc_log(LogDebug,
                "session router: linking new CNAME to existing SSRC:"
                " ssrc=%lu cname=%s",
                (unsigned long)source_id, rtcp::cname_to_str(cname).c_str());

        strcpy(source_route->cname, cname);

        if (!cname_route_map_.insert(source_route->cname_node)) {
            roc_log(LogError, "session router: allocation failed");

            strcpy(source_route->cname, "");
            return status::StatusNoMem;
        }

        return status::StatusOK;
    }

    // Only CNAME route exists.
    if (cname_route) {
        // Link SSRC to existing route.
        roc_log(LogDebug,
                "session router: linking new SSRC to existing CNAME:"
                " ssrc=%lu cname=%s",
                (unsigned long)source_id, rtcp::cname_to_str(cname).c_str());

        core::SharedPtr<SourceNode> node = new (source_node_pool_)
            SourceNode(source_node_pool_, *cname_route, source_id);

        if (!node) {
            roc_log(LogError, "session router: allocation failed");
            return status::StatusNoMem;
        }

        cname_route->source_nodes.push_back(*node);

        if (!source_route_map_.insert(*node)) {
            roc_log(LogError, "session router: allocation failed");

            cname_route->source_nodes.remove(*node);
            return status::StatusNoMem;
        }

        return status::StatusOK;
    }

    // Can't happen.
    roc_panic("session router: unreachable branch");
}

void ReceiverSessionRouter::unlink_source(packet::stream_source_t source_id) {
    // Find route for SSRC.
    SourceNode* node = source_route_map_.find(source_id);
    if (!node) {
        // Nothing to remove.
        roc_log(LogTrace,
                "session router: SSRC is not linked, nothing to unlink:"
                " ssrc=%lu",
                (unsigned long)source_id);

        return;
    }

    // Remember route before we remove SSRC node.
    Route& route = node->route();

    // Remove SSRC from route.
    roc_log(LogDebug, "session router: unlinking SSRC: ssrc=%lu n_ssrcs=%lu",
            (unsigned long)source_id, (unsigned long)node->route().source_nodes.size());

    source_route_map_.remove(*node);
    route.source_nodes.remove(*node);

    // Check if it was main SSRC.
    if (route.has_main_source_id && route.main_source_id == source_id) {
        route.has_main_source_id = false;
        route.main_source_id = 0;
    }

    // Remove route if needed.
    collect_route_(route);
}

status::StatusCode
ReceiverSessionRouter::relink_source_(packet::stream_source_t source_id,
                                      const char* cname) {
    // Remove SSRC from old route.
    roc_log(LogDebug, "session router: unlinking SSRC: ssrc=%lu",
            (unsigned long)source_id);

    SourceNode* old_node = source_route_map_.find(source_id);
    roc_panic_if(!old_node);
    Route& old_route = old_node->route();

    source_route_map_.remove(*old_node);
    old_node->route().source_nodes.remove(*old_node);

    // Link SSRC to new route.
    status::StatusCode code = link_source(source_id, cname);
    if (code != status::StatusOK) {
        return code;
    }

    if (old_route.has_main_source_id && old_route.main_source_id == source_id) {
        // If we're moving main SSRC from one route to another, we move session
        // and address too, because they are associated with this specific SSRC.
        SourceNode* new_node = source_route_map_.find(source_id);
        roc_panic_if(!new_node);
        Route& new_route = new_node->route();

        if ((code = move_route_session_(old_route, new_route)) != status::StatusOK) {
            remove_route_(&new_route);
            return code;
        }

        new_route.has_main_source_id = true;
        new_route.main_source_id = source_id;

        old_route.has_main_source_id = false;
        old_route.main_source_id = 0;
    }

    // Remove old route if needed.
    collect_route_(old_route);

    return status::StatusOK;
}

status::StatusCode
ReceiverSessionRouter::create_route_(const packet::stream_source_t source_id,
                                     const address::SocketAddr& source_addr,
                                     const char* cname,
                                     const core::SharedPtr<ReceiverSession>& session) {
    roc_log(LogDebug,
            "session router: creating route:"
            " ssrc=%lu cname=%s address=%s has_session=%d",
            (unsigned long)source_id, rtcp::cname_to_str(cname ? cname : "").c_str(),
            address::socket_addr_to_str(source_addr).c_str(), session ? 1 : 0);

    // Create route.
    core::SharedPtr<Route> route = new (route_pool_) Route(route_pool_);
    if (!route) {
        roc_log(LogError, "session router: allocation failed");
        return status::StatusNoMem;
    }
    route_list_.push_back(*route);

    // Add SSRC to route.
    {
        SourceNode* node =
            new (source_node_pool_) SourceNode(source_node_pool_, *route, source_id);
        if (!node) {
            roc_log(LogError, "session router: allocation failed");
            remove_route_(route);
            return status::StatusNoMem;
        }

        route->source_nodes.push_back(*node);

        if (!source_route_map_.insert(*node)) {
            roc_log(LogError, "session router: allocation failed");
            remove_route_(route);
            return status::StatusNoMem;
        }

        // Mark this SSRC as main.
        route->has_main_source_id = true;
        route->main_source_id = source_id;
    }

    // Add CNAME to route.
    if (cname) {
        roc_panic_if(strlen(cname) > rtcp::MaxCnameLen);
        strcpy(route->cname, cname);

        if (!cname_route_map_.insert(route->cname_node)) {
            roc_log(LogError, "session router: allocation failed");
            remove_route_(route);
            return status::StatusNoMem;
        }
    }

    // Add address to route.
    if (source_addr) {
        route->source_addr = source_addr;

        if (!address_route_map_.insert(route->address_node)) {
            roc_log(LogError, "session router: allocation failed");
            remove_route_(route);
            return status::StatusNoMem;
        }
    }

    // Add session to route.
    if (session) {
        route->session = session;

        if (!session_route_map_.insert(route->session_node)) {
            roc_log(LogError, "session router: allocation failed");
            remove_route_(route);
            return status::StatusNoMem;
        }
    }

    return status::StatusOK;
}

void ReceiverSessionRouter::remove_route_(core::SharedPtr<Route> route) {
    roc_log(LogDebug,
            "session router: removing route:"
            " n_ssrcs=%lu cname=%s address=%s has_session=%d",
            (unsigned long)route->source_nodes.size(),
            rtcp::cname_to_str(route->cname).c_str(),
            address::socket_addr_to_str(route->source_addr).c_str(),
            route->session ? 1 : 0);

    // Remove SSRCs from mappings.
    while (!route->source_nodes.is_empty()) {
        if (source_route_map_.contains(*route->source_nodes.back())) {
            source_route_map_.remove(*route->source_nodes.back());
        }
        route->source_nodes.remove(*route->source_nodes.back());
    }

    // Remove CNAME from mappings.
    if (cname_route_map_.contains(route->cname_node)) {
        cname_route_map_.remove(route->cname_node);
    }

    // Remove address from mappings.
    if (address_route_map_.contains(route->address_node)) {
        address_route_map_.remove(route->address_node);
    }

    // Remove session from mappings.
    if (session_route_map_.contains(route->session_node)) {
        session_route_map_.remove(route->session_node);
    }

    // Remove route.
    route_list_.remove(*route);
}

void ReceiverSessionRouter::remove_all_routes_() {
    while (!route_list_.is_empty()) {
        remove_route_(route_list_.back());
    }
}

status::StatusCode ReceiverSessionRouter::move_route_session_(Route& old_route,
                                                              Route& new_route) {
    roc_log(LogDebug, "session router: moving session to new route");

    // Move source address.
    if (address_route_map_.contains(old_route.address_node)) {
        address_route_map_.remove(old_route.address_node);
    }

    if (address_route_map_.contains(new_route.address_node)) {
        address_route_map_.remove(new_route.address_node);
    }

    new_route.source_addr = old_route.source_addr;
    old_route.source_addr.clear();

    if (!address_route_map_.insert(new_route.address_node)) {
        roc_log(LogError, "session router: allocation failed");
        return status::StatusNoMem;
    }

    // Move session.
    if (session_route_map_.contains(old_route.session_node)) {
        session_route_map_.remove(old_route.session_node);
    }

    if (session_route_map_.contains(new_route.session_node)) {
        session_route_map_.remove(new_route.session_node);
    }

    new_route.session = old_route.session;
    old_route.session = NULL;

    if (!session_route_map_.insert(new_route.session_node)) {
        roc_log(LogError, "session router: allocation failed");
        return status::StatusNoMem;
    }

    return status::StatusOK;
}

void ReceiverSessionRouter::collect_route_(Route& route) {
    // If we unlinked last SSRC, remove entire route.
    if (route.source_nodes.size() == 0) {
        roc_log(LogTrace,
                "session router:"
                " removed last SSRC, now removing entire route");

        remove_route_(&route);
    }
}

} // namespace pipeline
} // namespace roc
