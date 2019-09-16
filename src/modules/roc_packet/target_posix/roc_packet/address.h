/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/target_posix/roc_packet/address.h
//! @brief Network address.

#ifndef ROC_PACKET_ADDRESS_H_
#define ROC_PACKET_ADDRESS_H_

#include <netinet/in.h>
#include <sys/socket.h>

#include "roc_core/stddefs.h"

namespace roc {
namespace packet {

//! Network address.
class Address {
public:
    //! Construct invalid address.
    Address();

    //! Check if the address was properly initialized.
    bool valid() const;

    //! Set address from sockaddr struct.
    bool set_saddr(const sockaddr* sa);

    //! Set IPv4 host address.
    bool set_host_ipv4(const char* ip, int port);

    //! Set IPv6 host address.
    bool set_host_ipv6(const char* ip, int port);

    //! Set IPv4 address of the interface on which to join to the multicast group.
    bool set_miface_ipv4(const char* iface);

    //! Set IPv6 address of the interface on which to join to the multicast group.
    bool set_miface_ipv6(const char* iface);

    //! Get sockaddr struct.
    sockaddr* saddr();

    //! Get sockaddr struct.
    const sockaddr* saddr() const;

    //! Get sockaddr struct length.
    socklen_t slen() const;

    //! Get IP version (4 or 6).
    int version() const;

    //! Get address port.
    int port() const;

    //! Check whether this is multicast address.
    bool multicast() const;

    //! Check whether IP address of the interface on which to join to the
    //! multicast group is initialized.
    bool has_miface() const;

    //! Get host IP address.
    bool get_host(char* buf, size_t bufsz) const;

    //! Get IP address of the interface on which to join to the multicast group.
    bool get_miface(char* buf, size_t bufsz) const;

    //! Compare addresses.
    bool operator==(const Address& other) const;

    //! Compare addresses.
    bool operator!=(const Address& other) const;

    enum {
        // An estimate maximum length of a string representation of an address.
        MaxStrLen = 128
    };

private:
    static socklen_t sizeof_(sa_family_t family);

    sa_family_t family_() const;

    union {
        sockaddr_in addr4;
        sockaddr_in6 addr6;
    } sa_;

    union {
        in_addr addr4;
        in6_addr addr6;
    } miface_;

    sa_family_t miface_family_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_ADDRESS_H_
