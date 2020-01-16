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

#include "roc_address/socket_addr_enums.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace address {

//! Socket address.
class SocketAddr {
public:
    //! Construct empty address.
    SocketAddr();

    //! Check whether host and port are set.
    bool has_host_port() const;

    //! Set address from sockaddr struct.
    bool set_host_port_saddr(const sockaddr* sa);

    //! Set host address.
    bool set_host_port(AddrType type, const char*, int port);

    //! Set IPv4 host address.
    bool set_host_port_ipv4(const char* ip, int port);

    //! Set IPv6 host address.
    bool set_host_port_ipv6(const char* ip, int port);

    //! Check whether multicast interface address is set.
    bool has_miface() const;

    //! Set address of the interface on which to join to the multicast group.
    bool set_miface(AddrType type, const char* ip);

    //! Set IPv4 address of the interface on which to join to the multicast group.
    bool set_miface_ipv4(const char* ip);

    //! Set IPv6 address of the interface on which to join to the multicast group.
    bool set_miface_ipv6(const char* ip);

    //! Set broadcast flag.
    bool set_broadcast();

    //! Get IP version (4 or 6).
    int version() const;

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
        MaxStrLen = 128
    };

private:
    static socklen_t saddr_size_(sa_family_t family);

    sa_family_t saddr_family_() const;

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
