/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/print_supported.h"
#include "roc_address/protocol_map.h"
#include "roc_core/log.h"
#include "roc_core/printer.h"

namespace roc {
namespace address {

namespace {

enum { LineSize = 70 };

void print_interface_protos(core::Printer& prn,
                            Interface interface,
                            const core::StringList& list) {
    const char* str = list.front();

    while (str != NULL) {
        prn.writef(" ");

        size_t size = 0;

        prn.writef(" %s:", interface_to_str(interface));

        while (size < LineSize) {
            size += prn.writef(" %s%s%s", "", str, "://");

            str = list.nextof(str);
            if (!str) {
                break;
            }
        }

        prn.writef("\n");
    }
}

} // namespace

bool print_supported(ProtocolMap& protocol_map, core::IArena& arena) {
    core::Printer prn;
    core::Array<Interface> interface_array(arena);
    core::StringList list(arena);

    if (!protocol_map.get_supported_interfaces(interface_array)) {
        roc_log(LogError, "can't retrieve interface array");
        return false;
    }

    for (size_t n_interface = 0; n_interface < interface_array.size(); n_interface++) {
        if (!protocol_map.get_supported_protocols(interface_array[n_interface], list)) {
            roc_log(LogError, "can't retrieve protocols list");
            return false;
        }

        if (n_interface == 0) {
            prn.writef("\nsupported network protocols:\n");
        }

        print_interface_protos(prn, interface_array[n_interface], list);
    }
    return true;
}

} // namespace address
} // namespace roc
