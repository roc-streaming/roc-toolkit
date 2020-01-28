/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint_uri.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace address {

%%{
    machine parse_endpoint_uri;
    write data;
}%%

bool parse_endpoint_uri(const char* str, EndpointURI::Subset subset, EndpointURI& result) {
    result.clear(subset);

    if (!str) {
        roc_log(LogError, "parse endpoint uri: input string is null");

        result.invalidate(subset);
        return false;
    }

    // for ragel
    const char* p = str;
    const char *pe = str + strlen(str);

    const char *eof = pe;
    int cs = 0;

    // for actions
    EndpointProtocol proto = EndProto_None;
    const char* start_p = NULL;

    // parse result
    bool success = false;

    %%{
        action start_token {
            start_p = p;
        }

        action set_proto {
            if (subset != EndpointURI::Subset_Full) {
                roc_log(LogError,
                        "parse endpoint uri: unexpected scheme when parsing resource");
                return false;
            }
            if (!result.set_proto(proto)) {
                roc_log(LogError, "parse endpoint uri: invalid protocol");
                return false;
            }
        }

        action set_host {
            if (subset != EndpointURI::Subset_Full) {
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
            if (subset != EndpointURI::Subset_Full) {
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

        action set_fragment {
            if (!result.set_encoded_fragment(start_p, p - start_p)) {
                roc_log(LogError, "parse endpoint uri: invalid fragment");
                return false;
            }
        }

        scheme = 'rtsp'      %{ proto = EndProto_RTSP; }
               | 'rtp'       %{ proto = EndProto_RTP; }
               | 'rtp+rs8m'  %{ proto = EndProto_RTP_RS8M_Source; }
               | 'rs8m'      %{ proto = EndProto_RS8M_Repair; }
               | 'rtp+ldpc'  %{ proto = EndProto_RTP_LDPC_Source; }
               | 'ldpc'      %{ proto = EndProto_LDPC_Repair; }
               ;

        proto = scheme %set_proto;

        host = ('[' [^/@\[\]]+ ']' | [^/:@\[\]]+) >start_token %set_host;
        port = (digit+) >start_token %set_port;

        pchar = [^?#];

        path = ('/' pchar*) >start_token %set_path;
        query = (pchar*) >start_token %set_query;
        fragment = (pchar*) >start_token %set_fragment;

        uri = ( proto '://' host (':' port)? )? path? ('?' query)? ('#' fragment)?;

        main := uri
                %{ success = true; }
                ;

        write init;
        write exec;
    }%%

    if (!success) {
        if (subset == EndpointURI::Subset_Full) {
            roc_log(LogError,
                    "parse endpoint uri: expected"
                    " 'PROTO://HOST[:PORT][/PATH][?QUERY][#FRAGMENT]',\n"
                    " got '%s'",
                    str);
        } else {
            roc_log(LogError,
                    "parse endpoint uri: expected"
                    " '[/PATH][?QUERY][#FRAGMENT]',\n"
                    " got '%s'",
                    str);
        }
        result.invalidate(subset);
        return false;
    }

    if (!result.check(subset)) {
        result.invalidate(subset);
        return false;
    }

    return true;
}

} // namespace address
} // namespace roc
