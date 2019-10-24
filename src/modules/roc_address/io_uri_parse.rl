/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/io_uri.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace address {

%%{
    machine parse_io_uri;
    write data;
}%%

bool parse_io_uri(const char* str, IoURI& result) {
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

        action set_scheme {
            if (!result.set_scheme(start_p, p - start_p)) {
                roc_log(LogError, "parse io uri: invalid scheme");
                return false;
            }
        }

        action set_file_scheme {
            const char* scheme = "file";

            if (!result.set_scheme(scheme, strlen(scheme))) {
                roc_log(LogError, "parse io uri: invalid scheme");
                return false;
            }
        }

        action set_path {
            if (!result.set_encoded_path(start_p, p - start_p)) {
                roc_log(LogError, "parse io uri: invalid path");
                return false;
            }
        }

        pchar = [^?#];

        file_scheme = 'file' %set_file_scheme;

        abs_path = ('/' (pchar - '/') pchar*) >start_token %set_path;
        rel_path = ([^/] pchar*) >start_token %set_path;
        special_path = '-' >start_token %set_path;

        file_hier_part = ('//' 'localhost'?)? abs_path
                       | '//'? special_path
                       | (rel_path - special_path);

        file_uri = file_scheme ':' file_hier_part;

        device_scheme = (alnum+ - file_scheme) >start_token %set_scheme;
        device_hier_part = pchar+ >start_token %set_path;

        device_uri = device_scheme '://' device_hier_part;

        main := ( file_uri | device_uri )
                %{ success = true; }
                ;

        write init;
        write exec;
    }%%

    if (!success) {
        roc_log(LogError,
                "parse io uri: expected one of:\n"
                " 'DEVICE_TYPE://DEVICE_NAME',\n"
                " 'file:///ABS/PATH',\n"
                " 'file://localhost/ABS/PATH',\n"
                " 'file:/ABS/PATH',\n"
                " 'file:REL/PATH',\n"
                " 'file://-',\n"
                " 'file:-',\n"
                " got '%s'",
                str);
        return false;
    }

    return true;
}

} // namespace address
} // namespace roc
