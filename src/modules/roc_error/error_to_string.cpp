/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_error/error_to_string.h"

namespace roc {
namespace error {

const char* error_to_string(ErrorCode err) {
    switch (err) {
    case ErrUnknown:
        return "unknown error";
    }
    return "invalid error code";
}

} // namespace error
} // namespace roc
