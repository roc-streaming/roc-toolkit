/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_status/code_to_str.h"
#include "roc_status/status_code.h"

namespace roc {
namespace status {

const char* code_to_str(StatusCode code) {
    switch (code) {
    case StatusOK:
        return "OK";
    case StatusUnknown:
        return "Unknown";
    case StatusNoData:
        return "NoData";
    case StatusNoMem:
        return "NoMem";
    case StatusNoSpace:
        return "NoSpace";
    case StatusLimit:
        return "Limit";
    case StatusConflict:
        return "Conflict";
    }

    return "<invalid>";
}

} // namespace status
} // namespace roc
