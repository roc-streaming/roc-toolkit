/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sdp/media_description.h
//! @brief SDP Media Description.

#ifndef ROC_SDP_MEDIA_DESCRIPTION_H_
#define ROC_SDP_MEDIA_DESCRIPTION_H_

#include "roc_address/socket_addr.h"
#include "roc_core/attributes.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/log.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/string_buffer.h"
#include "roc_core/string_builder.h"
#include "roc_core/string_list.h"
#include "roc_sdp/connection_data.h"
#include "roc_sdp/media_transport.h"
#include "roc_sdp/media_type.h"

namespace roc {
namespace sdp {

//! SDP media description.
//! @code
//!  m=<type> <port> <proto> <fmt>
//! @endcode
class MediaDescription : public core::RefCounted<MediaDescription, core::ArenaAllocation>,
                         public core::ListNode<> {
public:
    //! Initialize empty media description
    MediaDescription(core::IArena& arena);

    //! Clear all fields.
    void clear();

    //! Media type.
    MediaType type() const;

    //! Transport port.
    int port() const;

    //! Number of transport port(s).
    int nb_ports() const;

    //! Transport protocol.
    MediaTransport transport() const;

    //! Default media payload id.
    unsigned default_payload_id() const;

    //! Number of payload ids.
    size_t nb_payload_ids() const;

    //! Get the payload id that was listed at the i position in the media description.
    unsigned payload_id(size_t i) const;

    //! Number of connection data.
    size_t nb_connection_data() const;

    //! Get the reference of the i-th connection data that was listed just after the media
    //! description.
    const ConnectionData& connection_data(size_t i) const;

    //! Set media type.
    ROC_ATTR_NODISCARD bool set_type(MediaType type);

    //! Set proto.
    ROC_ATTR_NODISCARD bool set_transport(MediaTransport transport);

    //! Set transport port.
    ROC_ATTR_NODISCARD bool set_port(long port);

    //! Set number of transport port(s).
    ROC_ATTR_NODISCARD bool set_nb_ports(long nb_ports);

    //! Add a media payload id.
    ROC_ATTR_NODISCARD bool add_payload_id(unsigned payload_id);

    //! Add a connection field from a string.
    ROC_ATTR_NODISCARD bool
    add_connection_data(address::AddrFamily addrtype, const char* str, size_t str_len);

private:
    MediaType type_;
    int port_;
    int nb_ports_;
    MediaTransport transport_;
    core::Array<unsigned, 2> payload_ids_;

    core::Array<ConnectionData, 1> connection_data_;
};

} // namespace sdp
} // namespace roc

#endif // ROC_SDP_MEDIA_DESCRIPTION_H_
