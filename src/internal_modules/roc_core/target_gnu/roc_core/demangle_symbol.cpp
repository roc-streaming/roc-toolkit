/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/backtrace.h"

#include <cxxabi.h>
#include <stdlib.h>
#include <string.h>

namespace roc {
namespace core {

const char*
demangle_symbol(const char* mangled, char*& demangled_buf, size_t& demangled_size) {
    if (demangled_buf == NULL) {
        // Using heap is dangerous when handling a crash, since it may be corrupted.
        // We can't completely avoid using heap because __cxa_demangle() relies on it.
        // So we at least try to reduce the number of reallocations by pre-allocating
        // a large enough buffer.
        size_t new_size = strlen(mangled) * 2;
        if (new_size < 128) {
            new_size = 128;
        }

        char* new_buf = (char*)malloc(new_size);
        if (!new_buf) {
            return NULL;
        }

        demangled_buf = new_buf;
        demangled_size = new_size;
    }

    // __cxa_demangle() will realloc() demangled_buf if it is too small and
    // update demangled_size accordingly
    int status = -1;
    char* new_buf = abi::__cxa_demangle(mangled, demangled_buf, &demangled_size, &status);

    if (status != 0 || !new_buf) {
        return NULL;
    }

    demangled_buf = new_buf;
    return new_buf;
}

} // namespace core
} // namespace roc
