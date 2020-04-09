/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sdp/media_description.h
//! @brief Media Description Protocol.

#ifndef ROC_SDP_MEDIA_DESCRIPTION_H_
#define ROC_SDP_MEDIA_DESCRIPTION_H_

#include "roc_address/socket_addr.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/log.h"
#include "roc_core/refcnt.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/string_buffer.h"
#include "roc_core/string_builder.h"
#include "roc_core/string_list.h"
#include "roc_sdp/connection_data.h"
#include "roc_sdp/media_proto.h"
#include "roc_sdp/media_type.h"

namespace roc {
namespace sdp {

//! SDP media description.
// m=<type> <port> <proto> <fmt>.
class MediaDescription : public core::RefCnt<MediaDescription>, public core::ListNode {
public:
    //! Clear all fields.
    void clear();

    //! Initialize empty media description
    MediaDescription(core::IAllocator& allocator);

    //! Media type.
    MediaType type() const;

    //! Transport port.
    int port() const;

    //! Number of transport port(s).
    int nb_ports() const;

    //! Transport protocol.
    MediaProto proto() const;

    //! Default media format for the session.
    const char* default_fmt() const;

    //! Set media type.
    bool set_type(MediaType type);

    //! Set proto.
    bool set_proto(MediaProto proto);

    //! Set transport port.
    bool set_port(int port);

    //! Set number of transport port(s).
    bool set_nb_ports(int nb_ports);

    //! Add a media format.
    //! String should not be zero-terminated.
    bool add_fmt(const char* str, size_t str_len);

    //! Add a connection field from a string.
    bool
    add_connection_data(address::AddrFamily addrtype, const char* str, size_t str_len);

private:
    friend class core::RefCnt<MediaDescription>;

    void destroy();

    MediaType type_;
    core::StringBuffer<> media_;
    int port_;
    int nb_ports_;
    MediaProto proto_;
    core::StringList fmts_;

    core::Array<ConnectionData, 1> connection_data_;

    core::IAllocator& allocator_;
};

} // namespace sdp
} // namespace roc

#endif // ROC_SDP_MEDIA_DESCRIPTION_H_
