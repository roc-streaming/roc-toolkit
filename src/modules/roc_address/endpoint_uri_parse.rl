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

bool parse_endpoint_uri(const char* str, EndpointURI& result) {
    roc_panic_if(str == NULL);

    result.clear();

    // for ragel
    const char* p = str;
    const char *pe = str + strlen(str);

    const char *eof = pe;
    int cs = 0;

    // for start_token
    const char* start_p = NULL;

    // parse result
    bool success = false;

    %%{
        action start_token {
            start_p = p;
        }

        action set_host {
            if (!result.set_encoded_host(start_p, p - start_p)) {
                roc_log(LogError, "parse endpoint uri: invalid host");
                return false;
            }
        }

        action set_port {
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

        scheme = 'rtsp'      %{ result.set_proto(EndProto_RTSP); }
               | 'rtp'       %{ result.set_proto(EndProto_RTP); }
               | 'rtp+rs8m'  %{ result.set_proto(EndProto_RTP_RS8M_Source); }
               | 'rs8m'      %{ result.set_proto(EndProto_RS8M_Repair); }
               | 'rtp+ldpc'  %{ result.set_proto(EndProto_RTP_LDPC_Source); }
               | 'ldpc'      %{ result.set_proto(EndProto_LDPC_Repair); }
               ;

        host = ('[' [^/@\[\]]+ ']' | [^/:@\[\]]+) >start_token %set_host;
        port = (digit+) >start_token %set_port;

        pchar = [^?#];

        path = ('/' pchar*) >start_token %set_path;
        query = (pchar*) >start_token %set_query;
        fragment = (pchar*) >start_token %set_fragment;

        uri = scheme '://' host (':' port)? path? ('?' query)? ('#' fragment)?;

        main := uri
                %{ success = true; }
                ;

        write init;
        write exec;
    }%%

    if (!success) {
        roc_log(LogError,
                "parse endpoint uri: expected"
                " 'PROTO://HOST[:PORT][/PATH][?QUERY][#FRAGMENT]',\n"
                " got '%s'",
                str);
        result.clear();
        return false;
    }

    if (!validate_endpoint_uri(result)) {
        result.clear();
        return false;
    }

    return true;
}

} // namespace address
} // namespace roc
