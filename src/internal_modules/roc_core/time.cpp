/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/stddefs.h"
#include "roc_core/time.h"

namespace roc {
namespace core {

bool ns_equal(nanoseconds_t t1, nanoseconds_t t2, nanoseconds_t epsilon) {
    nanoseconds_t abs_error = std::max(t1, t2) - std::min(t1, t2);
    return abs_error <= epsilon;
}

} // namespace core
} // namespace roc
