/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/target_berkley/roc_address/socket_addr.h
//! @brief Socket address.

#ifndef ROC_ADDRESS_SOCKET_ADDR_H_
#define ROC_ADDRESS_SOCKET_ADDR_H_

// splitter comment for clang-format
#ifndef __WIN32__
#include <netinet/in.h>
#include <sys/socket.h>
#else // ! __WIN32__
#include <cstdint>
typedef uint16_t sa_family_t;
typedef uint16_t in_port_t;
#include <winsock2.h>
#include <ws2tcpip.h>
#endif // __WIN32__
// splitter comment for clang-format

#include "roc_address/addr_family.h"
#include "roc_core/attributes.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace address {

//! Socket address.
class SocketAddr {
public:
    //! Construct empty address.
    SocketAddr();

    //! Clear address.
    void clear();

    //! Check whether host and port are set.
    bool has_host_port() const;

    //! Set host address.
    ROC_NODISCARD bool set_host_port(AddrFamily type, const char* host, int port);

    //! Set host address, auto-detect family.
    ROC_NODISCARD bool set_host_port_auto(const char* host, int port);

    //! Set address from sockaddr struct.
    ROC_NODISCARD bool set_host_port_saddr(const sockaddr* sa);

    //! Get IP version (IPv4 or IPv6).
    AddrFamily family() const;

    //! Check whether this is multicast address.
    bool is_multicast() const;

    //! Get host IP address.
    ROC_NODISCARD bool get_host(char* buf, size_t bufsz) const;

    //! Get address port.
    int port() const;

    //! Get sockaddr struct.
    sockaddr* saddr();

    //! Get sockaddr struct.
    const sockaddr* saddr() const;

    //! Get sockaddr struct length.
    socklen_t slen() const;

    //! Get maximum allowed sockaddr struct length.
    socklen_t max_slen() const;

    //! Convert to bool.
    operator const struct unspecified_bool *() const;

    //! Compare addresses.
    bool operator==(const SocketAddr& other) const;

    //! Compare addresses.
    bool operator!=(const SocketAddr& other) const;

    enum {
        // An estimate maximum length of a string representation of an address.
        MaxStrLen = 196
    };

private:
    static socklen_t saddr_size_(sa_family_t family);

    sa_family_t saddr_family_() const;

    bool set_host_port_ipv4_(const char* ip, int port);
    bool set_host_port_ipv6_(const char* ip, int port);

    union {
        sockaddr_in addr4;
        sockaddr_in6 addr6;
    } saddr_;
};

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_SOCKET_ADDR_H_
