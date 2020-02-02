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
#include "roc_core/log.h"
#include "roc_core/string_buffer.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace sdp {

//! SDP session description.
class SessionDescription : public core::NonCopyable<> {
public:
    //! Clear all fields.
    void clear();

    //! Initialize empty session description
    SessionDescription(core::IAllocator& allocator);

    //! Globally Unique Identifier
    const char* guid() const;

    //! Set GUID
    bool set_guid(
                  const char* start_p_origin_username,
                  const char* end_p_origin_sess_id,
                  const char* start_p_origin_nettype,
                  const char* end_p_origin_addr);

    //! Set origin address type
    bool set_origin_addrtype(address::AddrFamily addrtype);

    //! Check and set origin unicast address from a string.
    //! origin_addrtype should be defined (default: IP4)
    bool set_origin_unicast_address(const char* str, size_t str_len);

private:
    //! Tuple of username, sess-id, nettype, addrtype, and unicast-address forms a
    //! globally unique identifier for the session.
    core::StringBuffer<> guid_;

    //! Origin address type.
    address::AddrFamily origin_addrtype_;

    //! Origin unicast address.
    address::SocketAddr origin_unicast_address_;
};

//! Parse SDP session description from string.
bool parse_sdp(const char* str, SessionDescription& result);

} // namespace sdp
} // namespace roc

#endif // ROC_SDP_SESSION_DESCRIPTION_H_
