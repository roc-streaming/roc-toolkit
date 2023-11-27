/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/cname.h"
#include "roc_core/stddefs.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace rtcp {

cname_to_str::cname_to_str(const char* cname) {
    core::StringBuilder b(buffer_, sizeof(buffer_));

    if (cname) {
        const size_t cname_len = strlen(cname);

        bool is_printable = true;
        for (size_t i = 0; i < cname_len; i++) {
            if (!isascii(cname[i]) || !isprint(cname[i])) {
                is_printable = false;
                break;
            }
        }

        if (is_printable) {
            b.append_str("\"");
            b.append_str(cname);
            b.append_str("\"");
        } else {
            b.append_str("[");
            for (size_t i = 0; i < cname_len; i++) {
                if (i != 0) {
                    b.append_str(" ");
                }
                b.append_uint((unsigned char)cname[i], 16);
            }
            b.append_str("]");
        }
    } else {
        b.append_str("<null>");
    }
}

} // namespace rtcp
} // namespace roc
