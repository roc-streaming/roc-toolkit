/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/network_uri.h"
#include "roc_address/protocol_map.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace address {

%%{
    machine parse_network_uri;
    write data;
}%%

namespace {

bool parse_network_uri_imp(const char* str, NetworkUri::Subset subset, NetworkUri& result) {
    if (!str) {
        roc_log(LogError, "parse endpoint uri: input string is null");
        return false;
    }

    result.clear(subset);

    // for ragel
    const char* p = str;
    const char *pe = str + strlen(str);

    const char *eof = pe;
    int cs = 0;

    // for actions
    Protocol proto = Proto_None;
    const char* start_p = NULL;

    // parse result
    bool success = false;

    %%{
        action start_token {
            start_p = p;
        }

        action set_proto {
            char scheme[16] = {};
            if (p - start_p >= sizeof(scheme)) {
                roc_log(LogError, "parse endpoint uri: invalid protocol");
                return false;
            }
            strncpy(scheme, start_p, p - start_p);

            const ProtocolAttrs* attrs = ProtocolMap::instance().find_by_scheme(scheme);
            if (!attrs) {
                roc_log(LogError, "parse endpoint uri: invalid protocol");
                return false;
            }

            if (!result.set_proto(attrs->protocol)) {
                roc_log(LogError, "parse endpoint uri: invalid protocol");
                return false;
            }
        }

        action set_host {
            if (subset != NetworkUri::Subset_Full) {
                roc_log(LogError,
                        "parse endpoint uri: unexpected host when parsing resource");
                return false;
            }
            if (!result.set_host(start_p, p - start_p)) {
                roc_log(LogError, "parse endpoint uri: invalid host");
                return false;
            }
        }

        action set_port {
            if (subset != NetworkUri::Subset_Full) {
                roc_log(LogError,
                        "parse endpoint uri: unexpected port when parsing resource");
                return false;
            }

            char* end_p = NULL;
            long port = strtol(start_p, &end_p, 10);

            if (port == LONG_MAX || port == LONG_MIN || end_p != p) {
                roc_log(LogError, "parse endpoint uri: invalid port");
                return false;
            }

            if (!result.set_port((int)port)) {
                roc_log(LogError, "parse endpoint uri: invalid port");
                return false;
            }
        }

        action set_path {
            if (!result.set_encoded_path(start_p, p - start_p)) {
                roc_log(LogError, "parse endpoint uri: invalid path");
                return false;
            }
        }

        action set_query {
            if (!result.set_encoded_query(start_p, p - start_p)) {
                roc_log(LogError, "parse endpoint uri: invalid query");
                return false;
            }
        }

        proto = ( [a-z0-9+]+ ) >start_token %set_proto;

        host = ('[' [^/@\[\]]+ ']' | [^/:@\[\]]+) >start_token %set_host;
        port = (digit+) >start_token %set_port;

        pchar = [^?#];

        path = ('/' pchar*) >start_token %set_path;
        query = (pchar*) >start_token %set_query;

        uri = ( proto '://' host (':' port)? )? path? ('?' query)?;

        main := uri
                %{ success = true; }
                ;

        write init;
        write exec;
    }%%

    if (!success) {
        if (subset == NetworkUri::Subset_Full) {
            roc_log(LogError,
                    "parse endpoint uri: expected"
                    " 'PROTO://HOST[:PORT][/PATH][?QUERY]',\n"
                    " got '%s'",
                    str);
        } else {
            roc_log(LogError,
                    "parse endpoint uri: expected"
                    " '[/PATH][?QUERY]',\n"
                    " got '%s'",
                    str);
        }
        return false;
    }

    if (!result.verify(subset)) {
        roc_log(LogError, "parse endpoint uri: invalid uri");
        return false;
    }

    return true;
}

} // namespace

bool parse_network_uri(const char* str, NetworkUri::Subset subset, NetworkUri& result) {
    if (!parse_network_uri_imp(str, subset, result)) {
        result.invalidate(subset);
        return false;
    }
    return true;
}

} // namespace address
} // namespace roc
