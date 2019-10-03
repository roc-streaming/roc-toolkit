/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cxxabi.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unwind.h>

#include "roc_core/backtrace.h"
#include "roc_core/demangle.h"

namespace roc {
namespace core {

namespace {

enum { MaxLen = 128, MaxDepth = 128 };

struct BacktraceState {
    void** current;
    void** end;
};

_Unwind_Reason_Code unwind_callback(struct _Unwind_Context* context, void* arg) {
    BacktraceState* state = static_cast<BacktraceState*>(arg);
    _Unwind_Ptr pc = _Unwind_GetIP(context);
    if (pc) {
        if (state->current == state->end) {
            return _URC_END_OF_STACK;
        } else {
            *state->current++ = reinterpret_cast<void*>(pc);
        }
    }
    return _URC_NO_REASON;
}

ssize_t capture_backtrace(void** buffer, size_t max) {
    BacktraceState state = { buffer, buffer + max };
    _Unwind_Backtrace(unwind_callback, &state);
    if (state.current != NULL) {
        return state.current - buffer;
    }
    return 0;
}

void dump_backtrace(void** buffer, ssize_t count) {
    if (count <= 0) {
        fprintf(stderr, "No backtrace available\n");
    } else {
        fprintf(stderr, "Backtrace:\n");

        char* demangled_buf = NULL;
        size_t demangled_size = 0;

        for (ssize_t idx = 0; idx < count; ++idx) {
            const void* addr = buffer[idx];

            const char* symbol = "";
            const char* demangled_symbol = NULL;

            Dl_info info;
            if (dladdr(addr, &info) && info.dli_sname) {
                symbol = info.dli_sname;
                demangled_symbol = demangle(symbol, demangled_buf, demangled_size);
            }

            fprintf(stderr, "#%d: %p", (int)idx, addr);
            if (demangled_symbol) {
                fprintf(stderr, " %s\n", demangled_symbol);
            } else {
                fprintf(stderr, " %s\n", symbol);
            }
        }

        free(demangled_buf);
    }
}

} // namespace

void print_backtrace() {
    void* buffer[MaxDepth];
    dump_backtrace(buffer, capture_backtrace(buffer, MaxDepth));
}

void print_emergency_backtrace() {
}

} // namespace core
} // namespace roc
