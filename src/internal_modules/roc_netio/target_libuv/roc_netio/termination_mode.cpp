/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/termination_mode.h"
#include "roc_core/panic.h"

namespace roc {
namespace netio {

const char* termination_mode_to_str(TerminationMode mode) {
    switch (mode) {
    case Term_Normal:
        return "normal";

    case Term_Failure:
        return "failure";
    }

    roc_panic("unknown termination mode");
}

} // namespace netio
} // namespace roc
