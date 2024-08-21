/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/endpoint.h"

#include "adapters.h"
#include "arena.h"

#include "roc_address/network_uri.h"
#include "roc_core/log.h"

using namespace roc;

int roc_endpoint_allocate(roc_endpoint** result) {
    if (!result) {
        roc_log(LogError, "roc_endpoint_allocate(): invalid arguments: result is null");
        return -1;
    }

    address::NetworkUri* imp_endpoint =
        new (api::default_arena) address::NetworkUri(api::default_arena);

    if (!imp_endpoint) {
        roc_log(LogError, "roc_endpoint_allocate(): can't allocate endpoint");
        return -1;
    }

    *result = (roc_endpoint*)imp_endpoint;
    return 0;
}

int roc_endpoint_set_uri(roc_endpoint* endpoint, const char* uri) {
    if (!endpoint) {
        roc_log(LogError, "roc_endpoint_set_uri(): invalid arguments: endpoint is null");
        return -1;
    }

    address::NetworkUri& imp_endpoint = *(address::NetworkUri*)endpoint;

    if (!address::parse_network_uri(uri, address::NetworkUri::Subset_Full,
                                    imp_endpoint)) {
        roc_log(LogError, "roc_endpoint_set_uri(): invalid arguments: invalid uri");
        return -1;
    }

    return 0;
}

int roc_endpoint_set_protocol(roc_endpoint* endpoint, roc_protocol proto) {
    if (!endpoint) {
        roc_log(LogError,
                "roc_endpoint_set_protocol(): invalid arguments: endpoint is null");
        return -1;
    }

    address::NetworkUri& imp_endpoint = *(address::NetworkUri*)endpoint;

    address::Protocol imp_proto;
    if (!api::proto_from_user(imp_proto, proto)) {
        // set to invalid protocol to let NetworkUri invalidate protocol
        imp_proto = address::Proto_None;
    }

    if (!imp_endpoint.set_proto(imp_proto)) {
        roc_log(LogError,
                "roc_endpoint_set_protocol(): invalid arguments: invalid protocol");
        return -1;
    }

    return 0;
}

int roc_endpoint_set_host(roc_endpoint* endpoint, const char* host) {
    if (!endpoint) {
        roc_log(LogError, "roc_endpoint_set_host(): invalid arguments: endpoint is null");
        return -1;
    }

    address::NetworkUri& imp_endpoint = *(address::NetworkUri*)endpoint;

    if (!imp_endpoint.set_host(host)) {
        roc_log(LogError, "roc_endpoint_set_host(): can't set host");
        return -1;
    }

    return 0;
}

int roc_endpoint_set_port(roc_endpoint* endpoint, int port) {
    if (!endpoint) {
        roc_log(LogError, "roc_endpoint_set_port(): invalid arguments: endpoint is null");
        return -1;
    }

    address::NetworkUri& imp_endpoint = *(address::NetworkUri*)endpoint;

    if (!imp_endpoint.set_port(port)) {
        roc_log(LogError, "roc_endpoint_set_port(): invalid arguments: invalid port");
        return -1;
    }

    return 0;
}

int roc_endpoint_set_resource(roc_endpoint* endpoint, const char* encoded_resource) {
    if (!endpoint) {
        roc_log(LogError,
                "roc_endpoint_set_resource(): invalid arguments: endpoint is null");
        return -1;
    }

    address::NetworkUri& imp_endpoint = *(address::NetworkUri*)endpoint;

    if (!encoded_resource) {
        imp_endpoint.clear(address::NetworkUri::Subset_Resource);
        return 0;
    }

    if (!address::parse_network_uri(encoded_resource,
                                    address::NetworkUri::Subset_Resource, imp_endpoint)) {
        roc_log(LogError,
                "roc_endpoint_set_resource(): invalid arguments: invalid resource");
        return -1;
    }

    return 0;
}

int roc_endpoint_get_uri(const roc_endpoint* endpoint, char* buf, size_t* bufsz) {
    if (!endpoint) {
        roc_log(LogError, "roc_endpoint_get_uri(): invalid arguments: endpoint is null");
        return -1;
    }

    const address::NetworkUri& imp_endpoint = *(const address::NetworkUri*)endpoint;

    if (!bufsz) {
        roc_log(LogError, "roc_endpoint_get_uri(): invalid arguments: bufsz is null");
        return -1;
    }

    core::StringBuilder b(buf, *bufsz);

    if (!address::format_network_uri(imp_endpoint, address::NetworkUri::Subset_Full, b)) {
        roc_log(LogError, "roc_endpoint_get_uri(): endpoint uri is not set");
        return -1;
    }

    if (!b.is_ok()) {
        roc_log(LogError,
                "roc_endpoint_get_uri(): buffer too small: provided=%lu needed=%lu",
                (unsigned long)*bufsz, (unsigned long)b.needed_size());
        *bufsz = b.needed_size();
        return -1;
    }

    *bufsz = b.needed_size();
    return 0;
}

int roc_endpoint_get_protocol(const roc_endpoint* endpoint, roc_protocol* proto) {
    if (!endpoint) {
        roc_log(LogError,
                "roc_endpoint_get_protocol(): invalid arguments: endpoint is null");
        return -1;
    }

    const address::NetworkUri& imp_endpoint = *(const address::NetworkUri*)endpoint;

    if (!proto) {
        roc_log(LogError,
                "roc_endpoint_get_protocol(): invalid arguments: protocol is null");
        return -1;
    }

    address::Protocol imp_proto;
    if (!imp_endpoint.get_proto(imp_proto)) {
        roc_log(LogError, "roc_endpoint_get_protocol(): endpoint protocol is not set");
        return -1;
    }

    if (!api::proto_to_user(*proto, imp_proto)) {
        roc_log(LogError, "roc_endpoint_get_protocol(): endpoint protocol is invalid");
        return -1;
    }

    return 0;
}

int roc_endpoint_get_host(const roc_endpoint* endpoint, char* buf, size_t* bufsz) {
    if (!endpoint) {
        roc_log(LogError, "roc_endpoint_get_host(): invalid arguments: endpoint is null");
        return -1;
    }

    const address::NetworkUri& imp_endpoint = *(const address::NetworkUri*)endpoint;

    if (!bufsz) {
        roc_log(LogError, "roc_endpoint_get_host(): invalid arguments: bufsz is null");
        return -1;
    }

    core::StringBuilder b(buf, *bufsz);

    if (!imp_endpoint.format_host(b)) {
        roc_log(LogError, "roc_endpoint_get_host(): endpoint host is not set");
        return -1;
    }

    if (!b.is_ok()) {
        roc_log(LogError,
                "roc_endpoint_get_host(): buffer too small: provided=%lu needed=%lu",
                (unsigned long)*bufsz, (unsigned long)b.needed_size());
        *bufsz = b.needed_size();
        return -1;
    }

    *bufsz = b.needed_size();
    return 0;
}

int roc_endpoint_get_port(const roc_endpoint* endpoint, int* port) {
    if (!endpoint) {
        roc_log(LogError, "roc_endpoint_get_port(): invalid arguments: endpoint is null");
        return -1;
    }

    const address::NetworkUri& imp_endpoint = *(const address::NetworkUri*)endpoint;

    if (!port) {
        roc_log(LogError, "roc_endpoint_get_port(): invalid arguments: port is null");
        return -1;
    }

    if (!imp_endpoint.get_port(*port)) {
        roc_log(LogDebug, "roc_endpoint_get_port(): endpoint port is not set");
        return -1;
    }

    return 0;
}

int roc_endpoint_get_resource(const roc_endpoint* endpoint, char* buf, size_t* bufsz) {
    if (!endpoint) {
        roc_log(LogError,
                "roc_endpoint_get_resource(): invalid arguments: endpoint is null");
        return -1;
    }

    const address::NetworkUri& imp_endpoint = *(const address::NetworkUri*)endpoint;

    if (!bufsz) {
        roc_log(LogError,
                "roc_endpoint_get_resource(): invalid arguments: bufsz is null");
        return -1;
    }

    core::StringBuilder b(buf, *bufsz);

    if (!address::format_network_uri(imp_endpoint, address::NetworkUri::Subset_Resource,
                                     b)) {
        roc_log(LogDebug, "roc_endpoint_get_resource(): endpoint resource is not set");
        return -1;
    }

    if (!b.is_ok()) {
        roc_log(LogError,
                "roc_endpoint_get_resource(): buffer too small: provided=%lu needed=%lu",
                (unsigned long)*bufsz, (unsigned long)b.needed_size());
        *bufsz = b.needed_size();
        return -1;
    }

    *bufsz = b.needed_size();
    return 0;
}

int roc_endpoint_deallocate(roc_endpoint* endpoint) {
    if (!endpoint) {
        roc_log(LogError,
                "roc_endpoint_deallocate(): invalid arguments: endpoint is null");
        return -1;
    }

    address::NetworkUri& imp_endpoint = *(address::NetworkUri*)endpoint;
    api::default_arena.dispose_object(imp_endpoint);

    return 0;
}
