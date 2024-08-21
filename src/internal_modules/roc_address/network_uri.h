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
    //! URI subset.
    enum Subset {
        Subset_Full,    //!< Entire URI.
        Subset_Resource //!< Absolute path and query.
    };

    //! Initialize empty URI.
    explicit NetworkUri(core::IArena& arena);

    //! Check if URI is equivalent to another URI.
    bool is_equal(const NetworkUri& other) const;

    //! Copy data from another URI.
    ROC_ATTR_NODISCARD bool assign(const NetworkUri& other);

    //! Check given subset of the URI.
    bool verify(Subset subset) const;

    //! Clear given subset of the URI.
    void clear(Subset subset);

    //! Invalidate given subset of the URI.
    void invalidate(Subset subset);

    //! Set protocol ID (URI scheme).
    ROC_ATTR_NODISCARD bool set_proto(Protocol);

    //! Protocol ID (URI scheme).
    Protocol proto() const;

    //! Get protocol ID (URI scheme).
    ROC_ATTR_NODISCARD bool get_proto(Protocol& proto) const;

    //! Get URI proto.
    ROC_ATTR_NODISCARD bool format_proto(core::StringBuilder& dst) const;

    //! Set URI host.
    //! String should be zero-terminated.
    ROC_ATTR_NODISCARD bool set_host(const char* str);

    //! Set URI host.
    //! String should not be zero-terminated.
    ROC_ATTR_NODISCARD bool set_host(const char* str, size_t str_len);

    //! Hostname or IP address.
    const char* host() const;

    //! Get URI host.
    ROC_ATTR_NODISCARD bool format_host(core::StringBuilder& dst) const;

    //! Set port.
    ROC_ATTR_NODISCARD bool set_port(int);

    //! TCP or UDP port.
    int port() const;

    //! Get URI port.
    ROC_ATTR_NODISCARD bool get_port(int& port) const;

    //! Get string representation of port.
    //! If port is not set, default port for the protocol is used.
    //! This string is suitable for passing to getaddrinfo().
    //! @returns NULL if both port and default port are not set.
    const char* service() const;

    //! Set decoded URI path.
    ROC_ATTR_NODISCARD bool set_path(const char* str);

    //! Set decoded URI path.
    //! String should not be zero-terminated.
    ROC_ATTR_NODISCARD bool set_path(const char* str, size_t str_len);

    //! Set encoded URI path.
    //! String should be percent-encoded.
    ROC_ATTR_NODISCARD bool set_encoded_path(const char* str);

    //! Set encoded URI path.
    //! String should be percent-encoded.
    //! String should not be zero-terminated.
    ROC_ATTR_NODISCARD bool set_encoded_path(const char* str, size_t str_len);

    //! Decoded path.
    const char* path() const;

    //! Get URI path.
    //! String will be percent-encoded.
    ROC_ATTR_NODISCARD bool format_encoded_path(core::StringBuilder& dst) const;

    //! Set query.
    //! String should be percent-encoded.
    ROC_ATTR_NODISCARD bool set_encoded_query(const char* str);

    //! Set query.
    //! String should be percent-encoded.
    //! String should not be zero-terminated.
    ROC_ATTR_NODISCARD bool set_encoded_query(const char* str, size_t str_len);

    //! Raw query.
    const char* encoded_query() const;

    //! Get URI query.
    //! String will be percent-encoded.
    ROC_ATTR_NODISCARD bool format_encoded_query(core::StringBuilder& dst) const;

private:
    void set_service_from_port_(int port);
    bool set_service_from_proto_(Protocol proto);

    enum Part {
        PartProto = (1 << 0),
        PartHost = (1 << 1),
        PartPort = (1 << 2),
        PartPath = (1 << 3),
        PartQuery = (1 << 4)
    };

    bool part_is_valid_(Part part) const;
    void set_valid_(Part part);
    void set_invalid_(Part part);

    int invalid_parts_;

    Protocol proto_;

    core::StringBuffer host_;
    int port_;
    char service_[6];

    core::StringBuffer path_;
    core::StringBuffer query_;
};

//! Parse NetworkUri from string.
//!
//! The URI should be in the following form:
//!  - PROTOCOL://HOST[:PORT][/PATH][?QUERY]
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
ROC_ATTR_NODISCARD bool
parse_network_uri(const char* str, NetworkUri::Subset subset, NetworkUri& result);

//! Format NetworkUri to string.
//!
//! Formats a normalized form of the URI.
//!
//! The path and host parts of the URI are percent-encoded if necessary.
//! The query field is stored in the encoded form, so it is just copied as is.
//!
//! @returns
//!  true on success or false if the buffer is too small.
ROC_ATTR_NODISCARD bool format_network_uri(const NetworkUri& uri,
                                           NetworkUri::Subset subset,
                                           core::StringBuilder& dst);

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_NETWORK_URI_H_
