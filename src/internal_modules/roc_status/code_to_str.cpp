/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_status/code_to_str.h"

namespace roc {
namespace status {

const char* code_to_str(StatusCode code) {
    switch (code) {
    case StatusOK:
        return "OK";
    case StatusUnknown:
        return "unknown status";
    case StatusNoData:
        return "no data";
    default:
        break;
    }

    return "invalid status code";
}

} // namespace status
} // namespace roc
