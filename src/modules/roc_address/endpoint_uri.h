/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/endpoint_uri.h
//! @brief Network endpoint URI.

#ifndef ROC_ADDRESS_ENDPOINT_URI_H_
#define ROC_ADDRESS_ENDPOINT_URI_H_

#include "roc_address/endpoint_protocol.h"
#include "roc_address/endpoint_type.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_core/string_buffer.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace address {

//! Network endpoint URI.
class EndpointURI : public core::NonCopyable<> {
public:
    //! URI subset.
    enum Subset {
        Subset_Full,    //!< Entire URI.
        Subset_Resource //!< Absolute path + query + fragment.
    };

    //! Initialize empty URI.
    explicit EndpointURI(core::IAllocator&);

    //! Check given subset of the URI.
    bool check(Subset subset) const;

    //! Clear given subset of the URI.
    void clear(Subset subset);

    //! Invalidate given subset of the URI.
    void invalidate(Subset subset);

    //! Protocol ID (URI scheme).
    EndpointProtocol proto() const;

    //! Set protocol ID (URI scheme).
    bool set_proto(EndpointProtocol);

    //! Get protocol ID (URI scheme).
    bool get_proto(EndpointProtocol& proto) const;

    //! Hostname or IP address.
    const char* host() const;

    //! Set URI host.
    //! String should be zero-terminated.
    bool set_host(const char* str);

    //! Set URI host.
    //! String should not be zero-terminated.
    bool set_host(const char* str, size_t str_len);

    //! Get URI host.
    bool format_host(core::StringBuilder& dst) const;

    //! TCP or UDP port.
    int port() const;

    //! Set port.
    bool set_port(int);

    //! Get URI port.
    bool get_port(int& port) const;

    //! Get string representation of port.
    //! If port is not set, default port for the protocol is used.
    //! This string is suitable for passing to getaddrinfo().
    //! @returns NULL if both port and default port are not set.
    const char* service() const;

    //! Decoded path.
    const char* path() const;

    //! Set URI path.
    //! String should be percent-encoded.
    //! String should not be zero-terminated.
    bool set_encoded_path(const char* str, size_t str_len);

    //! Get URI path.
    //! String will be percent-encoded.
    bool format_encoded_path(core::StringBuilder& dst) const;

    //! Raw query.
    const char* encoded_query() const;

    //! Set query.
    //! String should be percent-encoded.
    //! String should not be zero-terminated.
    bool set_encoded_query(const char* str, size_t str_len);

    //! Get URI query.
    //! String will be percent-encoded.
    bool format_encoded_query(core::StringBuilder& dst) const;

    //! Raw fragment.
    const char* encoded_fragment() const;

    //! Set fragment.
    //! String should be percent-encoded.
    //! String should not be zero-terminated.
    bool set_encoded_fragment(const char* str, size_t str_len);

    //! Get URI fragment.
    //! String will be percent-encoded.
    bool format_encoded_fragment(core::StringBuilder& dst) const;

private:
    void set_service_from_port_(int port);
    bool set_service_from_proto_(EndpointProtocol proto);

    enum Part {
        PartProto = (1 << 0),
        PartHost = (1 << 1),
        PartPort = (1 << 2),
        PartPath = (1 << 3),
        PartQuery = (1 << 4),
        PartFrag = (1 << 5)
    };

    bool part_is_valid_(Part part) const;
    void set_valid_(Part part);
    void set_invalid_(Part part);

    int invalid_parts_;

    EndpointProtocol proto_;

    core::StringBuffer<56> host_;
    int port_;
    char service_[6];

    core::StringBuffer<> path_;
    core::StringBuffer<> query_;
    core::StringBuffer<> frag_;
};

//! Parse EndpointURI from string.
//!
//! The URI should be in the following form:
//!  - PROTOCOL://HOST[:PORT][/PATH][?QUERY][\#FRAGMENT]
//!
//! Examples:
//!  - rtp+rs8m://localhost
//!  - rtsp://localhost:123/path?query#frag
//!  - rtp://127.0.0.1:123
//!  - rtp://[::1]:123
//!
//! The URI syntax is defined by RFC 3986.
//!
//! The path, query, and fragment fields are allowed only for some protocols.
//!
//! The port field can be omitted if the protocol have standard port. Otherwise,
//! the port can not be omitted.
//!
//! The path and host fields of the URI are percent-decoded. (But the set of allowed
//! unencoded characters is different for path and host).
//!
//! The query and fragment fields of the URI are kept as is. The user is responsible
//! to percent-decode them when necessary.
//!
//! This parser does not try to perform full URI validation. For example, it does not
//! check that path contains only allowed symbols. If it can be parsed, it will be.
bool parse_endpoint_uri(const char* str, EndpointURI::Subset subset, EndpointURI& result);

//! Format EndpointURI to string.
//!
//! Formats a normalized form of the URI.
//!
//! The path and host parts of the URI are percent-encoded if necessary.
//!
//! The query and fragment parts are stored in the encoded form, so they
//! ar just copied as is.
//!
//! @returns
//!  true on success or false if the buffer is too small.
bool format_endpoint_uri(const EndpointURI& uri,
                         EndpointURI::Subset subset,
                         core::StringBuilder& dst);

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_ENDPOINT_URI_H_
