/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/backtrace.h"
#include "roc_core/console.h"

#include <cxxabi.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unwind.h>

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
        console_println("No backtrace available\n");
        return;
    }

    console_println("Backtrace:\n");

    char* demangled_buf = NULL;
    size_t demangled_size = 0;

    for (ssize_t idx = 0; idx < count; ++idx) {
        const void* addr = buffer[idx];

        const char* symbol = "";
        const char* demangled_symbol = NULL;

        Dl_info info;
        if (dladdr(addr, &info) && info.dli_sname) {
            symbol = info.dli_sname;
            demangled_symbol = demangle_symbol(symbol, demangled_buf, demangled_size);
        }

        console_println("#%d: %p %s", (int)idx, addr,
                        demangled_symbol ? demangled_symbol : symbol);
    }

    free(demangled_buf);
}

} // namespace

void print_backtrace_full() {
    void* buffer[MaxDepth];
    dump_backtrace(buffer, capture_backtrace(buffer, MaxDepth));
}

void print_backtrace_safe() {
    // not implemented
}

} // namespace core
} // namespace roc
