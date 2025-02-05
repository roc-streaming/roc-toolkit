/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/network_uri.h
//! @brief Network endpoint URI.

#ifndef ROC_ADDRESS_NETWORK_URI_H_
#define ROC_ADDRESS_NETWORK_URI_H_

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/attributes.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_core/string_buffer.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace address {

//! Network endpoint URI.
class NetworkUri : public core::NonCopyable<> {
public:
    //! URI field.
    enum Field {
        FieldProto = (1 << 0), //!< Scheme.
        FieldHost = (1 << 1),  //!< Host.
        FieldPort = (1 << 2),  //!< Optional port number.
        FieldPath = (1 << 3),  //!< Optional path.
        FieldQuery = (1 << 4), //!< Optional query.

        //! Full URI.
        FieldsAll = FieldProto | FieldHost | FieldPort | FieldPath | FieldQuery,
        //! Resource part of the URI.
        FieldsResource = FieldPath | FieldQuery,
    };

    //! Initialize empty URI.
    explicit NetworkUri(core::IArena& arena);

    //! Check if URI is equivalent to another URI.
    bool operator==(const NetworkUri& other) const;

    //! Check if URI is equivalent to another URI.
    bool operator!=(const NetworkUri& other) const;

    //! Check validity of the URI.
    //! @remarks
    //!  URI is valid if:
    //!  - No fields are invalidated.
    //!  - All required fields are present: protocol. host, and probably port:
    //!    whether port is required depends on protocol.
    //!  - No forbidden fields are present: whether path and query are allowed
    //!    depends on protocol.
    //! @note
    //!  Fields are invalidated explicitly by invalidate_fields() call, and
    //!  implicitly when a setter for that field fails.
    bool is_valid() const;

    //! Check if all of the fields from mask are present.
    //! @p field_mask is a bitmask of values from Field enum.
    bool has_fields(int fields_mask) const;

    //! Clear given fields of the URI.
    //! @p field_mask is a bitmask of values from Field enum.
    void clear_fields(int fields_mask);

    //! Mark given fields as invalid.
    //! @p field_mask is a bitmask of values from Field enum.
    void invalidate_fields(int fields_mask);

    //! Copy data from another URI.
    ROC_NODISCARD bool assign(const NetworkUri& other);

    //! Set protocol ID (URI scheme).
    ROC_NODISCARD bool set_proto(Protocol);

    //! Protocol ID (URI scheme).
    Protocol proto() const;

    //! Get protocol ID (URI scheme).
    ROC_NODISCARD bool get_proto(Protocol& proto) const;

    //! Get URI proto.
    ROC_NODISCARD bool format_proto(core::StringBuilder& dst) const;

    //! Set URI host.
    //! String should be zero-terminated.
    ROC_NODISCARD bool set_host(const char* str);

    //! Set URI host.
    //! String should not be zero-terminated.
    ROC_NODISCARD bool set_host(const char* str, size_t str_len);

    //! Hostname or IP address.
    const char* host() const;

    //! Get URI host.
    ROC_NODISCARD bool format_host(core::StringBuilder& dst) const;

    enum {
        //! Use default port number defined by protocol.
        DefautPort = -1
    };

    //! Set port.
    ROC_NODISCARD bool set_port(int port);

    //! TCP or UDP port.
    int port() const;

    //! Get URI port.
    ROC_NODISCARD bool get_port(int& port) const;

    //! Get port number, or default port number if port isn't set.
    int port_or_default() const;

    //! Set decoded URI path.
    ROC_NODISCARD bool set_path(const char* str);

    //! Set decoded URI path.
    //! String should not be zero-terminated.
    ROC_NODISCARD bool set_path(const char* str, size_t str_len);

    //! Set encoded URI path.
    //! String should be percent-encoded.
    ROC_NODISCARD bool set_encoded_path(const char* str);

    //! Set encoded URI path.
    //! String should be percent-encoded.
    //! String should not be zero-terminated.
    ROC_NODISCARD bool set_encoded_path(const char* str, size_t str_len);

    //! Decoded path.
    const char* path() const;

    //! Get URI path.
    //! String will be percent-encoded.
    ROC_NODISCARD bool format_encoded_path(core::StringBuilder& dst) const;

    //! Set query.
    //! String should be percent-encoded.
    ROC_NODISCARD bool set_encoded_query(const char* str);

    //! Set query.
    //! String should be percent-encoded.
    //! String should not be zero-terminated.
    ROC_NODISCARD bool set_encoded_query(const char* str, size_t str_len);

    //! Raw query.
    const char* encoded_query() const;

    //! Get URI query.
    //! String will be percent-encoded.
    ROC_NODISCARD bool format_encoded_query(core::StringBuilder& dst) const;

private:
    enum FieldState {
        NotEmpty,
        Empty,
        Broken,
    };

    FieldState field_state_(Field field) const;
    void set_field_state_(Field field, FieldState state);

    int non_empty_fields_;
    int broken_fields_;

    Protocol proto_;
    core::StringBuffer host_;
    int port_;
    core::StringBuffer path_;
    core::StringBuffer query_;
};

//! Parse network URI.
//!
//! The URI should be in the following form:
//! @code
//!   <proto>://<host>[:<port>][/<path>][?<query>]
//! @endcode
//!
//! Examples:
//!  - rtp+rs8m://localhost
//!  - rtsp://localhost:123/path?query
//!  - rtp://127.0.0.1:123
//!  - rtp://[::1]:123
//!
//! The URI syntax is defined by RFC 3986.
//!
//! The path and query fields are allowed only for some protocols.
//!
//! The port field can be omitted if the protocol have standard port. Otherwise,
//! the port can not be omitted.
//!
//! The path and host fields of the URI are percent-decoded. (But the set of allowed
//! unencoded characters is different for path and host).
//!
//! The query fields of the URI is kept as is. The user is responsible
//! to percent-decode it when necessary.
//!
//! This parser does not try to perform full URI validation. For example, it does not
//! check that path contains only allowed symbols. If it can be parsed, it will be.
ROC_NODISCARD bool parse_network_uri(const char* str, NetworkUri& result);

//! Parse resource part of network URI.
//!
//! Same as parse_network_uri(), but parses only path and query.
//! Keeps other fields untouched.
//! Fails if string contains anything besides path and query.
ROC_NODISCARD bool parse_network_uri_resource(const char* str, NetworkUri& result);

//! Format network URI.
//!
//! Formats a normalized form of the URI.
//!
//! The path and host parts of the URI are percent-encoded if necessary.
//! The query field is stored in the encoded form, so it is just copied as is.
//!
//! @returns
//!  true on success or false if the buffer is too small.
ROC_NODISCARD bool format_network_uri(const NetworkUri& uri, core::StringBuilder& dst);

//! Format resource part of network URI.
//!
//! Same as format_network_uri(), but formats only path and query.
//! Ignores other fields.
ROC_NODISCARD bool format_network_uri_resource(const NetworkUri& uri,
                                               core::StringBuilder& dst);

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_NETWORK_URI_H_
