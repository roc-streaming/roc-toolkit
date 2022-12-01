/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/backtrace.h"

namespace roc {
namespace core {

const char* demangle_symbol(const char*, char*&, size_t&) {
    return NULL;
}

} // namespace core
} // namespace roc
