/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sdp/session_description.h
//! @brief Session Description Protocol

#ifndef ROC_SDP_SESSION_DESCRIPTION_H_
#define ROC_SDP_SESSION_DESCRIPTION_H_

#include "roc_address/socket_addr.h"
#include "roc_core/array.h"
#include "roc_core/iallocator.h"
#include "roc_core/log.h"
#include "roc_core/string_buffer.h"
#include "roc_core/string_builder.h"
#include "roc_sdp/media_description.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/list.h"

namespace roc {
namespace sdp {

//! SDP session description.
class SessionDescription : public core::NonCopyable<> {
public:
    //! Clear all fields.
    void clear();

    //! Initialize empty session description
    SessionDescription(core::IAllocator& allocator);

    //! Globally Unique Identifier for the session.
    //! Built from a tuple of username, sess-id, nettype, addrtype, and unicast-address.
    const char* guid() const;

    //! Set GUID
    bool set_guid(
                  const char* start_p_origin_username,
                  const char* end_p_origin_sess_id,
                  const char* start_p_origin_nettype,
                  const char* end_p_origin_addr);


    //! Origin address type.
    address::AddrFamily origin_addrtype() const;

    //! Set origin address type
    bool set_origin_addrtype(address::AddrFamily addrtype);

    //! Origin unicast address.
    const address::SocketAddr origin_unicast_address() const;

    //! Check and set origin unicast address from a string.
    //! origin_addrtype should be defined (default: IP4)
    bool set_origin_unicast_address(const char* str, size_t str_len);

    //! Check and set session connection address from a string.
    bool set_session_connection_address(address::AddrFamily addrtype, const char* str, size_t str_len);

    //! Add a media description from a string.
    bool add_media_description(const char* str, size_t str_len);

    //! Get a shared pointer to the last added media description.
    const core::SharedPtr<MediaDescription> last_media_description() const;

    //! Check and add a connection address to the last added media from a string.
    bool add_connection_to_last_media(address::AddrFamily addrtype, const char* str, size_t str_len);


private:

    core::StringBuffer<> guid_;

    address::AddrFamily origin_addrtype_;     // TO REMOVE ?
    address::SocketAddr origin_unicast_address_;

    address::SocketAddr session_connection_address_;

    core::List<MediaDescription> media_descriptions_;

    core::IAllocator& allocator_;
};

//! Parse SDP session description from string.
bool parse_sdp(const char* str, SessionDescription& result);

} // namespace sdp
} // namespace roc

#endif // ROC_SDP_SESSION_DESCRIPTION_H_
