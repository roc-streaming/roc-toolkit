/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
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
    Address() {
        memset(&ss, 0, sizeof(ss));
    }

    //! Get sockaddr struct.
    sockaddr* saddr() const {
        return (sockaddr*)&ss;
    }

    //! Get sockaddr struct length.
    socklen_t slen() const {
        return sizeof_(ss.ss_family);
    }

    //! Copy given sockaddr struct to this address.
    bool set_saddr(const sockaddr* sa) {
        socklen_t slen = sizeof_(sa->sa_family);
        if (slen == 0) {
            return false;
        }
        memcpy(saddr(), sa, slen);
        return true;
    }

    //! Compare addresses.
    bool operator==(const Address& other) const {
        if (slen() != other.slen()) {
            return false;
        }
        if (slen() == 0) {
            return true;
        }
        return memcmp(saddr(), other.saddr(), slen()) == 0;
    }

    //! Compare addresses.
    bool operator!=(const Address& other) const {
        return !(*this == other);
    }

private:
    static socklen_t sizeof_(sa_family_t family) {
        switch (family) {
        case AF_INET:
            return sizeof(sockaddr_in);
        case AF_INET6:
            return sizeof(sockaddr_in6);
        default:
            return 0;
        }
    }

    sockaddr_storage ss;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_ADDRESS_H_
