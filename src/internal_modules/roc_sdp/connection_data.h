/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sdp/connection_data.h
//! @brief Connection field in a SDP.

#ifndef ROC_SDP_CONNECTION_DATA_H_
#define ROC_SDP_CONNECTION_DATA_H_

#include "roc_address/socket_addr.h"
#include "roc_core/attributes.h"
#include "roc_core/list_node.h"
#include "roc_core/log.h"
#include "roc_core/string_buffer.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace sdp {

//! SDP connection data field.
class ConnectionData {
public:
    //! Initialize empty connection data.
    ConnectionData();

    //! Clear all fields.
    void clear();

    //! Check and set connection address from a string.
    ROC_NODISCARD bool
    set_connection_address(address::AddrFamily addrtype, const char* str, size_t str_len);

    //! The SocketAddr of the ConnectionData.
    const address::SocketAddr& connection_address() const;

private:
    address::SocketAddr connection_address_;
};

} // namespace sdp
} // namespace roc

#endif // ROC_SDP_CONNECTION_DATA_H_
