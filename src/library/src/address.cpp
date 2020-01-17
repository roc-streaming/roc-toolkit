/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/address.h"

#include "address_helpers.h"

using namespace roc;

int roc_address_init(roc_address* address, roc_family family, const char* ip, int port) {
    if (sizeof(roc_address) < sizeof(address::SocketAddr)) {
        return -1;
    }

    if (!address) {
        return -1;
    }

    if (!ip) {
        return -1;
    }

    if (port < 0 || port > USHRT_MAX) {
        return -1;
    }

    address::SocketAddr& sa =
        *new (api::get_address_payload(address)) address::SocketAddr;

    if (family == ROC_AF_AUTO || family == ROC_AF_IPv4) {
        if (sa.set_host_port(address::Family_IPv4, ip, port)) {
            return 0;
        }
    }

    if (family == ROC_AF_AUTO || family == ROC_AF_IPv6) {
        if (sa.set_host_port(address::Family_IPv6, ip, port)) {
            return 0;
        }
    }

    return -1;
}

roc_family roc_address_family(const roc_address* address) {
    if (!address) {
        return ROC_AF_INVALID;
    }

    const address::SocketAddr& sa = api::get_socket_addr(address);

    switch ((unsigned)sa.version()) {
    case address::Family_IPv4:
        return ROC_AF_IPv4;
    case address::Family_IPv6:
        return ROC_AF_IPv6;
    default:
        break;
    }

    return ROC_AF_INVALID;
}

const char* roc_address_ip(const roc_address* address, char* buf, size_t bufsz) {
    if (!address) {
        return NULL;
    }

    if (!buf) {
        return NULL;
    }

    const address::SocketAddr& sa = api::get_socket_addr(address);

    if (!sa.get_host(buf, bufsz)) {
        return NULL;
    }

    return buf;
}

int roc_address_port(const roc_address* address) {
    if (!address) {
        return -1;
    }

    const address::SocketAddr& sa = api::get_socket_addr(address);

    int port = sa.port();
    if (port < 0) {
        return -1;
    }

    return port;
}
