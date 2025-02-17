/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_session_router.h
//! @brief Receiver session router.

#ifndef ROC_PIPELINE_RECEIVER_SESSION_ROUTER_H_
#define ROC_PIPELINE_RECEIVER_SESSION_ROUTER_H_

#include "roc_address/socket_addr.h"
#include "roc_core/attributes.h"
#include "roc_core/hashsum.h"
#include "roc_core/iarena.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/slab_pool.h"
#include "roc_packet/units.h"
#include "roc_pipeline/receiver_session.h"
#include "roc_rtcp/cname.h"
#include "roc_rtcp/headers.h"
#include "roc_status/status_code.h"

namespace roc {
namespace pipeline {

//! Receiver session router.
//!
//! Helps routing packets to sessions within session group.
//!
//! Session group corresponds to all sessions handled by one receiver slot - a set of
//! related complementary endpoints, e.g. one endpoint for audio, one for repair, and one
//! for control packets.
//!
//! For each remote sender, receiver creates a session inside session group. All audio,
//! repair, and control packets from same sender are then routed to same session.
//!
//! Session management is implemented in ReceiverSessionGroup, and algorithm for selecting
//! session for a packet is implemented in ReceiverSessionRouter (this class).
//!
//! Session router provides two methods to select session:
//!
//!  - By source id.
//!
//!    Sender can assign unique source id (SSRC) to each stream (audio, repair), and
//!    then transmit RTCP SDES packet that associate all sender's SSRCs with the same
//!    unique (randomly generated) CNAME string.
//!
//!    Session router will remember that these SSRCs are related and will route packets
//!    from those streams to same session.
//!
//!  - By source address.
//!
//!    As a fallback for the case when RTCP is not used, session router will assume that
//!    packets with same source address belong to the same session.
//!
//!    To make it work, sender should ensure that it sends all streams (audio, repair)
//!    from the same socket, and that there are no proxies or retranslators that combine
//!    multiple senders on the same socket.
class ReceiverSessionRouter : public core::NonCopyable<> {
public:
    //! Initialize.
    ReceiverSessionRouter(core::IArena& arena);

    //! Deinitialize.
    ~ReceiverSessionRouter();

    //! Get number of know routes.
    size_t num_routes();

    //! Find registered session by source id of sender's stream.
    //! @remarks
    //!  Sender can have multiple streams, each with its own SSRC.
    //!  Session router will remember all those SSRCs and map them to sender's session.
    //! @note
    //!  To make it work, one of the SSRCs should be explicitly mapped to a session
    //!  using add_session(), and the rest of SSRCs should be linked together using
    //!  link_source() with same CNAME. The order of these calls does not matter.
    core::SharedPtr<ReceiverSession> find_by_source(packet::stream_source_t source_id);

    //! Find registered session by source address of sender's stream.
    //! @remarks
    //!  Sender can use one source address for all its streams.
    //!  Session router will remember this address and map it to sender's session.
    //! @note
    //!  To make it work, sender's source address should be provided to add_session(),
    //!  and all sender's streams should have the same source address.
    core::SharedPtr<ReceiverSession>
    find_by_address(const address::SocketAddr& source_addr);

    //! Check if there is a route for given session.
    //! @remarks
    //!  Will return false after session was removed via remove_session()
    //!  or unlink_source().
    bool has_session(const core::SharedPtr<ReceiverSession>& session);

    //! Register session in router.
    //! @remarks
    //!  - @p session defines session where to route packets.
    //!  - @p source_id defines SSRC of stream which will be routed to the session.
    //!    Additional streams may be associated with same session via link_source() call.
    //!  - @p source_addr defines source address which will be routed to the session.
    //!    If other streams share the same source address, they will be routed to it.
    ROC_ATTR_NODISCARD status::StatusCode
    add_session(const core::SharedPtr<ReceiverSession>& session,
                packet::stream_source_t source_id,
                const address::SocketAddr& source_addr);

    //! Unregister session from router.
    //! @remarks
    //!  All associated SSRCs, CNAMEs, and addresses are removed.
    void remove_session(const core::SharedPtr<ReceiverSession>& session);

    //! Link source id with unique CNAME.
    //! @remarks
    //!  Remembers what SSRCs are linked together by sharing the same CNAME.
    //!  If/when one of the linked SSRCs is associated with a session using add_session(),
    //!  all linked SSRCs become being routed to that session.
    ROC_ATTR_NODISCARD status::StatusCode link_source(packet::stream_source_t source_id,
                                                      const char* cname);

    //! Unlink source id from session.
    //! @remarks
    //!  Removes association of SSRC with session and CNAME.
    //!  If this was the last SSRC, the whole route is removed.
    void unlink_source(packet::stream_source_t source_id);

private:
    enum { PreallocatedRoutes = 4, PreallocatedSources = 8 };

    struct Route;

    // Map route by source id (ssrc).
    // Allocated from pool.
    struct SourceNode : core::RefCounted<SourceNode, core::PoolAllocation>,
                        core::HashmapNode<>,
                        core::ListNode<> {
        Route& parent_route;
        const packet::stream_source_t source_id;

        SourceNode(core::IPool& pool, Route& route, packet::stream_source_t source_id)
            : core::RefCounted<SourceNode, core::PoolAllocation>(pool)
            , parent_route(route)
            , source_id(source_id) {
        }

        Route& route() {
            return parent_route;
        }

        packet::stream_source_t key() {
            // Use source id as a key.
            return source_id;
        }

        static core::hashsum_t key_hash(packet::stream_source_t source_id) {
            return core::hashsum_int(source_id);
        }

        static bool key_equal(packet::stream_source_t source_id1,
                              packet::stream_source_t source_id2) {
            return source_id1 == source_id2;
        }
    };

    // Map route by source address.
    // Embedded into Route struct.
    struct AddressNode : core::HashmapNode<> {
        Route& route() {
            return *ROC_CONTAINER_OF(this, Route, address_node);
        }

        const address::SocketAddr& key() {
            // Use route's source address as a key.
            return route().source_addr;
        }

        static core::hashsum_t key_hash(const address::SocketAddr& source_addr) {
            return core::hashsum_mem(source_addr.saddr(), (size_t)source_addr.slen());
        }

        static bool key_equal(const address::SocketAddr& source_addr1,
                              const address::SocketAddr& source_addr2) {
            return source_addr1 == source_addr2;
        }
    };

    // Map route by cname.
    // Embedded into Route struct.
    struct CnameNode : core::HashmapNode<> {
        Route& route() {
            return *ROC_CONTAINER_OF(this, Route, cname_node);
        }

        const char* key() {
            // Use route's cname as a key
            return route().cname;
        }

        static core::hashsum_t key_hash(const char* cname) {
            return core::hashsum_str(cname);
        }

        static bool key_equal(const char* cname1, const char* cname2) {
            return strcmp(cname1, cname2) == 0;
        }
    };

    // Map route by session pointer.
    // Embedded into Route struct.
    struct SessionNode : core::HashmapNode<> {
        Route& route() {
            return *ROC_CONTAINER_OF(this, Route, session_node);
        }

        const core::SharedPtr<ReceiverSession>& key() {
            // Use route's session pointer as a key.
            return route().session;
        }

        static core::hashsum_t key_hash(const core::SharedPtr<ReceiverSession>& session) {
            return core::hashsum_int((uintptr_t)session.get());
        }

        static bool key_equal(const core::SharedPtr<ReceiverSession>& session1,
                              const core::SharedPtr<ReceiverSession>& session2) {
            return session1 == session2;
        }
    };

    // Route represents one actual or potential receiver session.
    // Usually each route has a session, but it can be created before session if RTCP
    // packets come before RTP packets, and thus won't have a session for a while.
    //
    // Route can be mapped by different keys:
    //  - one or several source ids (SSRCs)
    //  - one source address
    //  - one cname
    //  - one session pointer
    //
    // All of the mappings are optional. Route struct owns hashmap nodes for all these
    // mappings, to allow including it into corresponding hash tables.
    struct Route : core::RefCounted<Route, core::PoolAllocation>, core::ListNode<> {
        // Session to which packets are routed.
        // May be empty.
        core::SharedPtr<ReceiverSession> session;

        // Sender source address.
        // May be empty.
        address::SocketAddr source_addr;

        // Sender cname.
        // May be empty.
        char cname[rtcp::MaxCnameLen + 1];

        // Sender main source ID.
        // Set to one of the identifiers from source_nodes list and
        // identifies source provided to add_session().
        bool has_main_source_id;
        packet::stream_source_t main_source_id;

        // Hashmap nodes to map this route by different keys.
        core::List<SourceNode> source_nodes;
        AddressNode address_node;
        CnameNode cname_node;
        SessionNode session_node;

        Route(core::IPool& pool)
            : core::RefCounted<Route, core::PoolAllocation>(pool)
            , has_main_source_id(false)
            , main_source_id(0) {
            cname[0] = '\0';
        }
    };

    status::StatusCode relink_source_(packet::stream_source_t source_id,
                                      const char* cname);

    status::StatusCode create_route_(const packet::stream_source_t source_id,
                                     const address::SocketAddr& source_addr,
                                     const char* cname,
                                     const core::SharedPtr<ReceiverSession>& session);
    void remove_route_(core::SharedPtr<Route> route);
    void remove_all_routes_();
    status::StatusCode move_route_session_(Route& from, Route& to);
    void collect_route_(Route& route);

    // Pools
    core::SlabPool<Route, PreallocatedRoutes> route_pool_;
    core::SlabPool<SourceNode, PreallocatedSources> source_node_pool_;

    // List of all routes
    // Holds ownership to routes to keep them alive
    core::List<Route> route_list_;

    // Mappings to find routes by different keys
    // Don't hold ownership to routes
    core::Hashmap<SourceNode, PreallocatedRoutes, core::NoOwnership> source_route_map_;
    core::Hashmap<AddressNode, PreallocatedRoutes, core::NoOwnership> address_route_map_;
    core::Hashmap<CnameNode, PreallocatedRoutes, core::NoOwnership> cname_route_map_;
    core::Hashmap<SessionNode, PreallocatedRoutes, core::NoOwnership> session_route_map_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SESSION_ROUTER_H_
