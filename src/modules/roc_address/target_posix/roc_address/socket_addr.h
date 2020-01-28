/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/target_posix/roc_address/socket_addr.h
//! @brief Socket address.

#ifndef ROC_ADDRESS_SOCKET_ADDR_H_
#define ROC_ADDRESS_SOCKET_ADDR_H_

#include <netinet/in.h>
#include <sys/socket.h>

#include "roc_address/addr_family.h"
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

    //! Set address from sockaddr struct.
    bool set_host_port_saddr(const sockaddr* sa);

    //! Set host address.
    bool set_host_port(AddrFamily type, const char* host, int port);

    //! Check whether multicast interface address is set.
    bool has_miface() const;

    //! Set address of the interface on which to join to the multicast group.
    bool set_miface(AddrFamily type, const char* ip);

    //! Set broadcast flag.
    bool set_broadcast();

    //! Get IP version (IPv4 or IPv6).
    AddrFamily family() const;

    //! Check whether this is multicast address.
    bool multicast() const;

    //! Check whether this is broadcast address.
    bool broadcast() const;

    //! Get host IP address.
    bool get_host(char* buf, size_t bufsz) const;

    //! Get IP address of the interface on which to join to the multicast group.
    bool get_miface(char* buf, size_t bufsz) const;

    //! Get address port.
    int port() const;

    //! Get sockaddr struct.
    sockaddr* saddr();

    //! Get sockaddr struct.
    const sockaddr* saddr() const;

    //! Get sockaddr struct length.
    socklen_t slen() const;

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

    bool set_miface_ipv4_(const char* ip);
    bool set_miface_ipv6_(const char* ip);

    union {
        sockaddr_in addr4;
        sockaddr_in6 addr6;
    } saddr_;

    sa_family_t miface_family_;

    union {
        in_addr addr4;
        in6_addr addr6;
    } miface_;

    bool broadcast_;
};

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_SOCKET_ADDR_H_
