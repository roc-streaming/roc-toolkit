/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sdp/session_description.h
//! @brief Session Description Protocol.

#ifndef ROC_SDP_SESSION_DESCRIPTION_H_
#define ROC_SDP_SESSION_DESCRIPTION_H_

#include "roc_address/socket_addr.h"
#include "roc_core/array.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_core/list.h"
#include "roc_core/log.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/string_buffer.h"
#include "roc_core/string_builder.h"
#include "roc_sdp/connection_data.h"
#include "roc_sdp/media_description.h"

namespace roc {
namespace sdp {

//! SDP session description.
class SessionDescription : public core::NonCopyable<> {
public:
    //! Initialize empty session description
    SessionDescription(core::IArena& arena);

    //! Clear all fields.
    void clear();

    //! Globally Unique Identifier for the session.
    //! Built from a tuple of username, sess-id, nettype, addrtype, and unicast-address.
    const char* guid() const;

    //! Set GUID
    ROC_NODISCARD bool set_guid(const char* start_p_origin_username,
                                const char* end_p_origin_username,
                                const char* start_p_origin_sess_id,
                                const char* end_p_origin_sess_id,
                                const char* start_p_origin_nettype,
                                const char* end_p_origin_nettype,
                                const char* start_p_origin_addr,
                                const char* end_p_origin_addr);

    //! Origin unicast address.
    const address::SocketAddr& origin_unicast_address() const;

    //! Check and set origin unicast address from a string.
    ROC_NODISCARD bool set_origin_unicast_address(address::AddrFamily addrtype,
                                                  const char* str,
                                                  size_t str_len);

    //! Check and set session connection address from a string.
    ROC_NODISCARD bool set_session_connection_data(address::AddrFamily addrtype,
                                                   const char* str,
                                                   size_t str_len);

    //! Get reference to the connection data of the session.
    const ConnectionData& session_connection_data();

    //! Create and add a new empty media description.
    ROC_NODISCARD bool add_media_description();

    //! Get a shared pointer to the last added media description.
    const core::SharedPtr<MediaDescription> last_media_description() const;

    //! Get a shared pointer to the first added media description.
    const core::SharedPtr<MediaDescription> first_media_description() const;

    //! Get a const reference to the list of media descriptions.
    const core::List<MediaDescription> get_media_descriptions() const;

    //! Get list media description next to given one.
    //!
    //! @returns
    //!  list media description following @p element if @p element is not
    //!  last, or NULL otherwise.
    //!
    //! @pre
    //!  @p element should be member of the list of media descriptions.
    const core::SharedPtr<MediaDescription>
    nextof_media_description(core::SharedPtr<MediaDescription> element) const;

private:
    core::StringBuffer guid_;

    address::SocketAddr origin_unicast_address_;

    ConnectionData session_connection_data_;

    core::List<MediaDescription> media_descriptions_;

    core::IArena& arena_;
};

//! Parse SDP session description from string.
bool parse_sdp(const char* str, SessionDescription& result);

} // namespace sdp
} // namespace roc

#endif // ROC_SDP_SESSION_DESCRIPTION_H_
