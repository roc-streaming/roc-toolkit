/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/print_supported.h"
#include "roc_core/log.h"
#include "roc_core/printer.h"
#include "roc_sndio/backend_dispatcher.h"

namespace roc {
namespace sndio {

namespace {

enum { LineSize = 70 };

void print_string_list(core::Printer& prn,
                       const core::StringList& list,
                       const char* prefix,
                       const char* suffix) {
    const char* str = list.front();

    while (str != NULL) {
        prn.writef(" ");

        size_t size = 0;
        while (size < LineSize) {
            size += prn.writef(" %s%s%s", prefix, str, suffix);

            str = list.nextof(str);
            if (!str) {
                break;
            }
        }

        prn.writef("\n");
    }
}

} // namespace

bool print_supported(BackendDispatcher& backend_dispatcher, core::IArena& arena) {
    core::StringList list(arena);
    core::Printer prn;

    if (!backend_dispatcher.get_supported_schemes(list)) {
        roc_log(LogError, "can't retrieve driver list");
        return false;
    }

    prn.writef("\nsupported schemes for audio devices and files:\n");
    print_string_list(prn, list, "", "://");

    if (!backend_dispatcher.get_supported_formats(list)) {
        roc_log(LogError, "can't retrieve format list");
        return false;
    }

    prn.writef("\nsupported formats for audio files:\n");
    print_string_list(prn, list, "", "");

    return true;
}

} // namespace sndio
} // namespace roc
