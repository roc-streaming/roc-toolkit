/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <arpa/inet.h>

#include "roc_address/socket_addr.h"

namespace roc {
namespace address {

SocketAddr::SocketAddr()
    : miface_family_(AF_UNSPEC)
    , broadcast_(false) {
    memset(&saddr_, 0, sizeof(saddr_));
    memset(&miface_, 0, sizeof(miface_));
}

bool SocketAddr::has_host_port() const {
    return saddr_family_() == AF_INET || saddr_family_() == AF_INET6;
}

bool SocketAddr::set_host_port_saddr(const sockaddr* sa) {
    const socklen_t sa_size = saddr_size_(sa->sa_family);
    if (sa_size == 0) {
        return false;
    }

    memcpy(&saddr_, sa, sa_size);

    return true;
}

bool SocketAddr::set_host_port(AddrFamily type, const char* ip_str, int port) {
    switch ((unsigned)type) {
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

    if (broadcast_ && IN_MULTICAST(ntohl(addr.s_addr))) {
        return false;
    }

    saddr_.addr4.sin_family = AF_INET;
    saddr_.addr4.sin_addr = addr;
    saddr_.addr4.sin_port = htons(uint16_t(port));

    return true;
}

bool SocketAddr::set_host_port_ipv6_(const char* ip_str, int port) {
    in6_addr addr;
    if (inet_pton(AF_INET6, ip_str, &addr) != 1) {
        return false;
    }

    if (broadcast_ && IN6_IS_ADDR_MULTICAST(&addr)) {
        return false;
    }

    saddr_.addr6.sin6_family = AF_INET6;
    saddr_.addr6.sin6_addr = addr;
    saddr_.addr6.sin6_port = htons(uint16_t(port));

    return true;
}

bool SocketAddr::set_miface(AddrFamily type, const char* iface) {
    switch ((unsigned)type) {
    case Family_IPv4:
        return set_miface_ipv4_(iface);
    case Family_IPv6:
        return set_miface_ipv6_(iface);
    default:
        return false;
    }
}

bool SocketAddr::set_miface_ipv4_(const char* iface) {
    in_addr addr;

    if (inet_pton(AF_INET, iface, &addr) != 1) {
        return false;
    }
    miface_.addr4 = addr;
    miface_family_ = AF_INET;

    return true;
}

bool SocketAddr::set_miface_ipv6_(const char* iface) {
    in6_addr addr;

    if (inet_pton(AF_INET6, iface, &addr) != 1) {
        return false;
    }
    miface_.addr6 = addr;
    miface_family_ = AF_INET6;

    return true;
}

bool SocketAddr::set_broadcast() {
    if (multicast()) {
        return false;
    }
    broadcast_ = true;
    return true;
}

sockaddr* SocketAddr::saddr() {
    return (sockaddr*)&saddr_;
}

const sockaddr* SocketAddr::saddr() const {
    return (const sockaddr*)&saddr_;
}

socklen_t SocketAddr::slen() const {
    return saddr_size_(saddr_family_());
}

AddrFamily SocketAddr::family() const {
    switch (saddr_family_()) {
    case AF_INET:
        return Family_IPv4;
    case AF_INET6:
        return Family_IPv6;
    default:
        return Family_Unknown;
    }
}

int SocketAddr::port() const {
    switch (saddr_family_()) {
    case AF_INET:
        return ntohs(saddr_.addr4.sin_port);
    case AF_INET6:
        return ntohs(saddr_.addr6.sin6_port);
    default:
        return -1;
    }
}

bool SocketAddr::multicast() const {
    switch (saddr_family_()) {
    case AF_INET:
        return IN_MULTICAST(ntohl(saddr_.addr4.sin_addr.s_addr));
    case AF_INET6:
        return IN6_IS_ADDR_MULTICAST(&saddr_.addr6.sin6_addr);
    default:
        return false;
    }
}

bool SocketAddr::broadcast() const {
    return broadcast_;
}

bool SocketAddr::has_miface() const {
    return miface_family_ == AF_INET || miface_family_ == AF_INET6;
}

bool SocketAddr::get_host(char* buf, size_t bufsz) const {
    switch (saddr_family_()) {
    case AF_INET:
        if (!inet_ntop(AF_INET, &saddr_.addr4.sin_addr, buf, (socklen_t)bufsz)) {
            return false;
        }
        break;

    case AF_INET6:
        if (!inet_ntop(AF_INET6, &saddr_.addr6.sin6_addr, buf, (socklen_t)bufsz)) {
            return false;
        }
        break;

    default:
        return false;
    }

    return true;
}

bool SocketAddr::get_miface(char* buf, size_t bufsz) const {
    switch (miface_family_) {
    case AF_INET:
        if (!inet_ntop(AF_INET, &miface_.addr4, buf, (socklen_t)bufsz)) {
            return false;
        }

        break;

    case AF_INET6:
        if (!inet_ntop(AF_INET6, &miface_.addr6, buf, (socklen_t)bufsz)) {
            return false;
        }

        break;

    default:
        return false;
    }

    return true;
}

bool SocketAddr::operator==(const SocketAddr& other) const {
    if (saddr_family_() != other.saddr_family_()) {
        return false;
    }

    switch (saddr_family_()) {
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

    if (miface_family_ != other.miface_family_) {
        return false;
    }

    switch (miface_family_) {
    case AF_INET:
        if (miface_.addr4.s_addr != other.miface_.addr4.s_addr) {
            return false;
        }

        break;
    case AF_INET6:
        if (memcmp(miface_.addr6.s6_addr, other.miface_.addr6.s6_addr,
                   sizeof(miface_.addr6.s6_addr))
            != 0) {
            return false;
        }

        break;

    default:
        break;
    }

    if (broadcast_ != other.broadcast_) {
        return false;
    }

    return true;
}

bool SocketAddr::operator!=(const SocketAddr& other) const {
    return !(*this == other);
}

socklen_t SocketAddr::saddr_size_(sa_family_t family) {
    switch (family) {
    case AF_INET:
        return sizeof(sockaddr_in);
    case AF_INET6:
        return sizeof(sockaddr_in6);
    default:
        return 0;
    }
}

sa_family_t SocketAddr::saddr_family_() const {
    return saddr_.addr4.sin_family;
}

} // namespace address
} // namespace roc
