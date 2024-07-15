/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/endpoint.h
 * \brief Network endpoint.
 */

#ifndef ROC_ENDPOINT_H_
#define ROC_ENDPOINT_H_

#include "roc/config.h"
#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Network endpoint.
 *
 * Endpoint is a network entry point of a peer. The definition includes the
 * protocol being used, network host and port, and, for some protocols, a
 * resource. All these parts together are unambiguously represented
 * by a URI. The user may set or get the entire URI or its individual parts.
 *
 * **Endpoint URI**
 *
 * Endpoint URI syntax is a subset of the syntax defined in RFC 3986:
 *
 *     protocol://host[:port][/path][?query]
 *
 * Examples:
 *  - `rtsp://localhost:123/path?query`
 *  - `rtp+rs8m://localhost:123`
 *  - `rtp://127.0.0.1:123`
 *  - `rtp://[::1]:123`
 *
 * The following protocols (schemes) are supported:
 *  - `rtp://`       (\ref ROC_PROTO_RTP)
 *  - `rtp+rs8m://`  (\ref ROC_PROTO_RTP_RS8M_SOURCE)
 *  - `rs8m://`      (\ref ROC_PROTO_RS8M_REPAIR)
 *  - `rtp+ldpc://`  (\ref ROC_PROTO_RTP_LDPC_SOURCE)
 *  - `ldpc://`      (\ref ROC_PROTO_LDPC_REPAIR)
 *
 * The host field should be either FQDN (domain name), or IPv4 address, or
 * IPv6 address in square brackets.
 *
 * The port field can be omitted if the protocol defines standard port. Otherwise,
 * the port can not be omitted. For example, RTSP defines standard port,
 * but RTP doesn't.
 *
 * The path and query fields are allowed only for protocols that support them.
 * For example, they're supported by RTSP, but not by RTP.
 *
 * **Field invalidation**
 *
 * If some field is attempted to be set to an invalid value (for example, an invalid
 * port number), this specific field is marked as invalid until it is successfully set
 * to some valid value.
 *
 * Sender and receiver refuse to bind or connect an endpoint which has invalid fields
 * or doesn't have some mandatory fields. Hence, it is safe to ignore errors returned by
 * endpoint setters and check only for errors returned by bind and connect operations.
 *
 * **Thread safety**
 *
 * Should not be used concurrently.
 */
typedef struct roc_endpoint roc_endpoint;

/** Allocate and initialize endpoint.
 *
 * All components of the newly created endpoint are empty.
 *
 * **Parameters**
 *  - \p result should point to an uninitialized roc_endpoint pointer
 *
 * **Returns**
 *  - returns zero if endpoint was successfully allocated and initialized
 *  - returns a negative value on invalid arguments
 *  - returns a negative value on allocation failure
 *
 * **Ownership**
 *  - passes the ownership of \p result to the user; the user is responsible to call
 *    roc_endpoint_deallocate() to free it
 */
ROC_API int roc_endpoint_allocate(roc_endpoint** result);

/** Set endpoint URI.
 *
 * Parses and sets endpoint URI. Overrides or clears each URI component.
 *
 * On failure, invalidates endpoint URI. The endpoint becomes invalid until its
 * URI or every individual component is successfully set.
 *
 * **Parameters**
 *  - \p endpoint should point to initialized endpoint
 *  - \p uri should be a valid endpoint URI
 *
 * **Returns**
 *  - returns zero if URI was successfully parsed and set
 *  - returns a negative value on invalid arguments
 *  - returns a negative value on allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p uri; it may be safely deallocated
 *    after the function returns
 */
ROC_API int roc_endpoint_set_uri(roc_endpoint* endpoint, const char* uri);

/** Set endpoint protocol (scheme).
 *
 * On failure, invalidates endpoint protocol. The endpoint becomes invalid until its
 * protocol is successfully set.
 *
 * **Parameters**
 *  - \p endpoint should point to initialized endpoint
 *  - \p proto defines new protocol
 *
 * **Returns**
 *  - returns zero if protocol was successfully set
 *  - returns a negative value on invalid arguments
 *  - returns a negative value if protocol is incompatible with other URI parameters
 */
ROC_API int roc_endpoint_set_protocol(roc_endpoint* endpoint, roc_protocol proto);

/** Set endpoint host.
 *
 * On failure, invalidates endpoint host. The endpoint becomes invalid until its
 * host is successfully set.
 *
 * **Parameters**
 *  - \p endpoint should point to initialized endpoint
 *  - \p host specifies FQDN, IPv4 address, or IPv6 address
 *
 * **Returns**
 *  - returns zero if host was successfully set
 *  - returns a negative value on invalid arguments
 *  - returns a negative value on allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p host; it may be safely deallocated
 *    after the function returns
 */
ROC_API int roc_endpoint_set_host(roc_endpoint* endpoint, const char* host);

/** Set endpoint port.
 *
 * When binding an endpoint, the port may be set to zero to select a random port.
 * The selected port will be then written back to the endpoint. When connecting
 * an endpoint, the port should be positive.
 *
 * If port is not set, the standard port for endpoint protocol is used. This is
 * allowed only if the protocol defines its standard port.
 *
 * If port is already set, it can be unset by setting to special value "-1".
 *
 * On failure, invalidates endpoint port. The endpoint becomes invalid until its
 * port is successfully set.
 *
 * **Parameters**
 *  - \p endpoint should point to initialized endpoint
 *  - \p port specifies UDP or TCP port in range [0; 65535]
 *
 * **Returns**
 *  - returns zero if port was successfully set
 *  - returns a negative value on invalid arguments
 */
ROC_API int roc_endpoint_set_port(roc_endpoint* endpoint, int port);

/** Set endpoint resource (path and query).
 *
 * Path and query are both optional. Any of them may be omitted. If path
 * is present, it should be absolute.
 *
 * The given resource should be percent-encoded by user if it contains special
 * characters. It may be inserted into the URI as is.
 *
 * If resource is already set, it can be unset by setting to NULL or "".
 *
 * On failure, invalidates endpoint resource. The endpoint becomes invalid until its
 * resource is successfully set.
 *
 * **Parameters**
 *  - \p endpoint should point to initialized endpoint
 *  - \p encoded_resource specifies percent-encoded path and query
 *
 * **Returns**
 *  - returns zero if resource was successfully set
 *  - returns a negative value on invalid arguments
 *  - returns a negative value on allocation failure
 *
 * **Ownership**
 *  - doesn't take or share the ownership of \p encoded_resource; it may be safely
 *    deallocated after the function returns
 */
ROC_API int roc_endpoint_set_resource(roc_endpoint* endpoint,
                                      const char* encoded_resource);

/** Get endpoint URI.
 *
 * Formats endpoint URI to user-provided buffer.
 *
 * If the function succeeds, the output string is zero-terminated. No matter whether
 * the function succeeds or fails, \p bufsz is updated with the actual output string
 * length, including terminating zero byte. \p buf may be NULL; in this case nothing
 * is written, but \p bufsz is still updated. This can be used to determine the
 * proper buffer size in before.
 *
 * **Parameters**
 *  - \p endpoint should point to initialized endpoint
 *  - \p buf should point to a buffer of \p bufsz bytes
 *  - \p bufsz defines the buffer size
 *
 * **Returns**
 *  - returns zero if URI was successfully formatted
 *  - returns a negative value on invalid arguments
 *  - returns a negative value if endpoint URI is not set
 *  - returns a negative value if buffer is too small
 */
ROC_API int roc_endpoint_get_uri(const roc_endpoint* endpoint, char* buf, size_t* bufsz);

/** Get endpoint protocol (scheme).
 *
 * **Parameters**
 *  - \p endpoint should point to initialized endpoint
 *  - \p proto should point to a variable where to write the protocol
 *
 * **Returns**
 *  - returns zero if protocol was successfully written
 *  - returns a negative value if endpoint protocol is not set
 *  - returns a negative value on invalid arguments
 */
ROC_API int roc_endpoint_get_protocol(const roc_endpoint* endpoint, roc_protocol* proto);

/** Get endpoint host.
 *
 * Formats endpoint URI host to user-provided buffer.
 *
 * If the function succeeds, the output string is zero-terminated. No matter whether
 * the function succeeds or fails, \p bufsz is updated with the actual output string
 * length, including terminating zero byte. \p buf may be NULL; in this case nothing
 * is written, but \p bufsz is still updated. This can be used to determine the
 * proper buffer size in before.
 *
 * **Parameters**
 *  - \p endpoint should point to initialized endpoint
 *  - \p buf should point to a buffer of \p bufsz bytes
 *  - \p bufsz defines the buffer size
 *
 * **Returns**
 *  - returns zero if host was successfully formatted
 *  - returns a negative value if endpoint host is not set
 *  - returns a negative value on invalid arguments
 *  - returns a negative value if buffer is too small
 */
ROC_API int roc_endpoint_get_host(const roc_endpoint* endpoint, char* buf, size_t* bufsz);

/** Get endpoint port.
 *
 * **Parameters**
 *  - \p endpoint should point to initialized endpoint
 *  - \p port should point to a variable where to write the port
 *
 * **Returns**
 *  - returns zero if port was successfully written
 *  - returns a negative value if endpoint port is not set
 *  - returns a negative value on invalid arguments
 */
ROC_API int roc_endpoint_get_port(const roc_endpoint* endpoint, int* port);

/** Get endpoint resource (path and query).
 *
 * Formats endpoint URI resource to user-provided buffer. The written
 * resource is percent-encoded.
 *
 * If the function succeeds, the output string is zero-terminated. No matter whether
 * the function succeeds or fails, \p bufsz is updated with the actual output string
 * length, including terminating zero byte. \p buf may be NULL; in this case nothing
 * is written, but \p bufsz is still updated. This can be used to determine the
 * proper buffer size in before.
 *
 * **Parameters**
 *  - \p endpoint should point to initialized endpoint
 *  - \p buf should point to a buffer of \p bufsz bytes
 *  - \p bufsz defines the buffer size
 *
 * **Returns**
 *  - returns zero if resource was successfully formatted
 *  - returns a negative value if neither of endpoint path and query is set
 *  - returns a negative value on invalid arguments
 *  - returns a negative value if buffer is too small
 */
ROC_API int
roc_endpoint_get_resource(const roc_endpoint* endpoint, char* buf, size_t* bufsz);

/** Deinitialize and deallocate endpoint.
 *
 * **Parameters**
 *  - \p endpoint should point to initialized endpoint
 *
 * **Returns**
 *  - returns zero if endpoint was successfully deallocated
 *  - returns a negative value on invalid arguments
 *
 * **Ownership**
 *  - ends the user ownership of \p endpoint; it can't be used anymore after the
 *    function returns
 */
ROC_API int roc_endpoint_deallocate(roc_endpoint* endpoint);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_ENDPOINT_H_ */
