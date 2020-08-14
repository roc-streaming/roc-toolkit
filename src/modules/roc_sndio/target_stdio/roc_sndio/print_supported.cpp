/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_core/log.h"
#include "roc_sndio/backend_dispatcher.h"
#include "roc_sndio/print_supported.h"

namespace roc {
namespace sndio {

namespace {

enum { ArraySize = 100, LineSize = 70 };

void print_string_list(const core::StringList& list,
                       const char* prefix,
                       const char* suffix) {
    const char* str = list.front();

    while (str != NULL) {
        printf(" ");

        int size = 0;
        while (size < LineSize) {
            int ret = printf(" %s%s%s", prefix, str, suffix);
            if (ret > 0) {
                size += ret;
            }

            str = list.nextof(str);
            if (!str) {
                break;
            }
        }

        printf("\n");
    }
}

} // namespace

bool print_supported(core::IAllocator& allocator, BackendDispatcher& backend_dispatcher) {
    core::StringList list(allocator);

    if (!backend_dispatcher.get_supported_schemes(list)) {
        roc_log(LogError, "can't retrieve driver list");
        return false;
    }

    printf("supported schemes for audio devices and files:\n");
    print_string_list(list, "", "://");

    if (!backend_dispatcher.get_supported_formats(list)) {
        roc_log(LogError, "can't retrieve format list");
        return false;
    }

    printf("\nsupported formats for audio files:\n");
    print_string_list(list, ".", "");

    return true;
}

} // namespace sndio
} // namespace roc
