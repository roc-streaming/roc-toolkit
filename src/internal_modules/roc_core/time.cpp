/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/time.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

bool ns_equal_delta(nanoseconds_t a, nanoseconds_t b, nanoseconds_t delta) {
    nanoseconds_t abs_error = std::max(a, b) - std::min(a, b);
    return abs_error <= delta;
}

} // namespace core
} // namespace roc
