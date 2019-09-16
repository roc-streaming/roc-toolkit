/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <arpa/inet.h>

#include "roc_packet/address.h"

namespace roc {
namespace packet {

Address::Address()
    : miface_family_(AF_UNSPEC) {
    memset(&sa_, 0, sizeof(sa_));
    memset(&miface_, 0, sizeof(miface_));
}

bool Address::valid() const {
    return family_() == AF_INET || family_() == AF_INET6;
}

bool Address::set_saddr(const sockaddr* sa) {
    const socklen_t sa_size = sizeof_(sa->sa_family);
    if (sa_size == 0) {
        return false;
    }

    memcpy(&sa_, sa, sa_size);

    return true;
}

bool Address::set_host_ipv4(const char* ip_str, int port) {
    in_addr addr;
    if (inet_pton(AF_INET, ip_str, &addr) != 1) {
        return false;
    }

    sa_.addr4.sin_family = AF_INET;
    sa_.addr4.sin_addr = addr;
    sa_.addr4.sin_port = htons(uint16_t(port));

    return true;
}

bool Address::set_host_ipv6(const char* ip_str, int port) {
    in6_addr addr;
    if (inet_pton(AF_INET6, ip_str, &addr) != 1) {
        return false;
    }

    sa_.addr6.sin6_family = AF_INET6;
    sa_.addr6.sin6_addr = addr;
    sa_.addr6.sin6_port = htons(uint16_t(port));

    return true;
}

bool Address::set_miface_ipv4(const char* iface) {
    in_addr addr;

    if (inet_pton(AF_INET, iface, &addr) != 1) {
        return false;
    }
    miface_.addr4 = addr;
    miface_family_ = AF_INET;

    return true;
}

bool Address::set_miface_ipv6(const char* iface) {
    in6_addr addr;

    if (inet_pton(AF_INET6, iface, &addr) != 1) {
        return false;
    }
    miface_.addr6 = addr;
    miface_family_ = AF_INET6;

    return true;
}

sockaddr* Address::saddr() {
    return (sockaddr*)&sa_;
}

const sockaddr* Address::saddr() const {
    return (const sockaddr*)&sa_;
}

socklen_t Address::slen() const {
    return sizeof_(family_());
}

int Address::version() const {
    switch (family_()) {
    case AF_INET:
        return 4;
    case AF_INET6:
        return 6;
    default:
        return -1;
    }
}

int Address::port() const {
    switch (family_()) {
    case AF_INET:
        return ntohs(sa_.addr4.sin_port);
    case AF_INET6:
        return ntohs(sa_.addr6.sin6_port);
    default:
        return -1;
    }
}

bool Address::multicast() const {
    switch (family_()) {
    case AF_INET:
        return IN_MULTICAST(ntohl(sa_.addr4.sin_addr.s_addr));
    case AF_INET6:
        return IN6_IS_ADDR_MULTICAST(&sa_.addr6.sin6_addr);
    default:
        return false;
    }
}

bool Address::has_miface() const {
    return miface_family_ == AF_INET || miface_family_ == AF_INET6;
}

bool Address::get_host(char* buf, size_t bufsz) const {
    switch (family_()) {
    case AF_INET:
        if (!inet_ntop(AF_INET, &sa_.addr4.sin_addr, buf, (socklen_t)bufsz)) {
            return false;
        }
        break;

    case AF_INET6:
        if (!inet_ntop(AF_INET6, &sa_.addr6.sin6_addr, buf, (socklen_t)bufsz)) {
            return false;
        }
        break;

    default:
        return false;
    }

    return true;
}

bool Address::get_miface(char* buf, size_t bufsz) const {
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

bool Address::operator==(const Address& other) const {
    if (family_() != other.family_()) {
        return false;
    }

    switch (family_()) {
    case AF_INET:
        if (sa_.addr4.sin_addr.s_addr != other.sa_.addr4.sin_addr.s_addr) {
            return false;
        }
        if (sa_.addr4.sin_port != other.sa_.addr4.sin_port) {
            return false;
        }
        break;

    case AF_INET6:
        if (memcmp(sa_.addr6.sin6_addr.s6_addr, other.sa_.addr6.sin6_addr.s6_addr,
                   sizeof(sa_.addr6.sin6_addr.s6_addr))
            != 0) {
            return false;
        }
        if (sa_.addr6.sin6_port != other.sa_.addr6.sin6_port) {
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

    return true;
}

bool Address::operator!=(const Address& other) const {
    return !(*this == other);
}

socklen_t Address::sizeof_(sa_family_t family) {
    switch (family) {
    case AF_INET:
        return sizeof(sockaddr_in);
    case AF_INET6:
        return sizeof(sockaddr_in6);
    default:
        return 0;
    }
}

sa_family_t Address::family_() const {
    return sa_.addr4.sin_family;
}

} // namespace packet
} // namespace roc
