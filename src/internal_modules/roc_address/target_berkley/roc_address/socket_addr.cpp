/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef ROC_TARGET_POSIX
#include <arpa/inet.h>
#endif

#include "roc_address/socket_addr.h"
#include "roc_core/endian.h"

namespace {

#ifdef ROC_TARGET_WINDOWS
typedef ADDRESS_FAMILY sa_family_t;
typedef USHORT in_port_t;
typedef size_t ntop_bufsz;
#else
typedef socklen_t ntop_bufsz;
#endif

socklen_t saddr_size(sa_family_t family) {
    switch (family) {
    case AF_INET:
        return sizeof(sockaddr_in);
    case AF_INET6:
        return sizeof(sockaddr_in6);
    default:
        return 0;
    }
}

sa_family_t saddr_family(const sockaddr_in& sa4) {
    return (sa_family_t)sa4.sin_family;
}

} // namespace

namespace roc {
namespace address {

SocketAddr::SocketAddr() {
    clear();
}

void SocketAddr::clear() {
    memset(&saddr_, 0, sizeof(saddr_));
}

bool SocketAddr::has_host_port() const {
    return saddr_family(saddr_.addr4) == AF_INET
        || saddr_family(saddr_.addr4) == AF_INET6;
}

bool SocketAddr::set_host_port_saddr(const sockaddr* sa) {
    const socklen_t sa_size = saddr_size(sa->sa_family);
    if (sa_size == 0) {
        return false;
    }

    memcpy(&saddr_, sa, (size_t)sa_size);

    return true;
}

bool SocketAddr::set_host_port_auto(const char* host, int port) {
    return set_host_port(Family_IPv4, host, port)
        || set_host_port(Family_IPv6, host, port);
}

bool SocketAddr::set_host_port(AddrFamily type, const char* ip_str, int port) {
    switch (type) {
    case Family_IPv4:
        return set_host_port_ipv4_(ip_str, port);
    case Family_IPv6:
        return set_host_port_ipv6_(ip_str, port);
    default:
        return false;
    }
}

bool SocketAddr::set_host_port_ipv4_(const char* ip_str, int port) {
    in_addr addr;
    if (inet_pton(AF_INET, ip_str, &addr) != 1) {
        return false;
    }

    saddr_.addr4.sin_family = AF_INET;
    saddr_.addr4.sin_addr = addr;
    saddr_.addr4.sin_port = (in_port_t)core::hton16u((uint16_t)port);

    return true;
}

bool SocketAddr::set_host_port_ipv6_(const char* ip_str, int port) {
    in6_addr addr;
    if (inet_pton(AF_INET6, ip_str, &addr) != 1) {
        return false;
    }

    saddr_.addr6.sin6_family = AF_INET6;
    saddr_.addr6.sin6_addr = addr;
    saddr_.addr6.sin6_port = (in_port_t)core::hton16u((uint16_t)port);

    return true;
}

sockaddr* SocketAddr::saddr() {
    return (sockaddr*)&saddr_;
}

const sockaddr* SocketAddr::saddr() const {
    return (const sockaddr*)&saddr_;
}

socklen_t SocketAddr::slen() const {
    return saddr_size(saddr_family(saddr_.addr4));
}

socklen_t SocketAddr::max_slen() const {
    return saddr_size(AF_INET6);
}

AddrFamily SocketAddr::family() const {
    switch (saddr_family(saddr_.addr4)) {
    case AF_INET:
        return Family_IPv4;
    case AF_INET6:
        return Family_IPv6;
    default:
        return Family_Unknown;
    }
}

int SocketAddr::port() const {
    switch (saddr_family(saddr_.addr4)) {
    case AF_INET:
        return core::ntoh16u((uint16_t)saddr_.addr4.sin_port);
    case AF_INET6:
        return core::ntoh16u((uint16_t)saddr_.addr6.sin6_port);
    default:
        return -1;
    }
}

#ifndef ROC_TARGET_WINDOWS
#define IN_MULTICAST_U(i) IN_MULTICAST(i) 
#else
// Defined as (((long)(i) & 0xf0000000) == 0xe0000000) in Windows, causes Wsign-conversion
#define IN_MULTICAST_U(i) (((unsigned long)(i) & 0xf0000000u) == 0xe0000000u)
#endif

bool SocketAddr::is_multicast() const {
    switch (saddr_family(saddr_.addr4)) {
    case AF_INET:
        return IN_MULTICAST_U(core::ntoh32u(saddr_.addr4.sin_addr.s_addr));
    case AF_INET6:
        return IN6_IS_ADDR_MULTICAST(&saddr_.addr6.sin6_addr);
    default:
        return false;
    }
}

bool SocketAddr::get_host(char* buf, size_t bufsz) const {
    switch (saddr_family(saddr_.addr4)) {
    case AF_INET:
        if (!inet_ntop(AF_INET, &saddr_.addr4.sin_addr, buf, (ntop_bufsz)bufsz)) {
            return false;
        }
        break;

    case AF_INET6:
        if (!inet_ntop(AF_INET6, &saddr_.addr6.sin6_addr, buf, (ntop_bufsz)bufsz)) {
            return false;
        }
        break;

    default:
        return false;
    }

    return true;
}

SocketAddr::operator const struct unspecified_bool *() const {
    return (const unspecified_bool*)has_host_port();
}

bool SocketAddr::operator==(const SocketAddr& other) const {
    if (saddr_family(saddr_.addr4) != saddr_family(other.saddr_.addr4)) {
        return false;
    }

    switch (saddr_family(saddr_.addr4)) {
    case AF_INET:
        if (saddr_.addr4.sin_addr.s_addr != other.saddr_.addr4.sin_addr.s_addr) {
            return false;
        }
        if (saddr_.addr4.sin_port != other.saddr_.addr4.sin_port) {
            return false;
        }
        break;

    case AF_INET6:
        if (memcmp(saddr_.addr6.sin6_addr.s6_addr, other.saddr_.addr6.sin6_addr.s6_addr,
                   sizeof(saddr_.addr6.sin6_addr.s6_addr))
            != 0) {
            return false;
        }
        if (saddr_.addr6.sin6_port != other.saddr_.addr6.sin6_port) {
            return false;
        }
        break;

    default:
        break;
    }

    return true;
}

bool SocketAddr::operator!=(const SocketAddr& other) const {
    return !(*this == other);
}

} // namespace address
} // namespace roc
