/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sdp/connection_field.h
//! @brief Connection field in a SDP

#ifndef ROC_SDP_CONNECTION_FIELD_H_
#define ROC_SDP_CONNECTION_FIELD_H_

#include "roc_address/socket_addr.h"
#include "roc_core/log.h"
#include "roc_core/string_buffer.h"
#include "roc_core/string_builder.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"

namespace roc {
namespace sdp {

//! SDP connection field for a media description.
class ConnectionField : public core::RefCnt<ConnectionField>, 
                         public core::ListNode {
public:
    //! Clear all fields.
    void clear();

    //! Initialize empty media description
    ConnectionField(core::IAllocator& allocator);

    //! Check and set connection address from a string.
    bool set_connection_address(address::AddrFamily addrtype, const char* str, size_t str_len);

private:
    friend class core::RefCnt<ConnectionField>;

    void destroy();

    address::SocketAddr connection_address_;    
    core::IAllocator& allocator_;
};

} // namespace sdp
} // namespace roc

#endif // ROC_SDP_CONNECTION_FIELD_H_
