/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cxxabi.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "roc_core/backtrace.h"

namespace roc {
namespace core {

namespace {

enum { MaxDepth = 128, DefaultLen = 128 };

struct WarmUp {
    WarmUp() {
        // This call initializes glibc backtrace module so that it performs all necessary
        // memory allocations at program start. We need this to ensure that backtrace()
        // call from a signal handler will not invoke malloc().
        void* nop;
        (void)backtrace(&nop, 1);
    }
};

WarmUp warmup;

} // namespace

// This function tries to performs symbol demangling, which works only with -rdynamic
// GCC option. This option is enabled in debug builds.
void print_backtrace() {
    void* array[MaxDepth] = {};
    int size = backtrace(array, MaxDepth);

    if (size <= 0) {
        fprintf(stderr, "No backtrace available\n");
        return;
    }

    fprintf(stderr, "Backtrace:\n");

    char** strings = backtrace_symbols(array, size);

    char* mangled = (char*)malloc(DefaultLen);
    size_t mangled_size = DefaultLen;

    char* demangled = (char*)malloc(DefaultLen);
    size_t demangled_size = DefaultLen;

    for (int i = 0; i < size; i++) {
        char* left_paren = NULL;
        char* right_paren = NULL;
        char* plus = NULL;

        for (char* p = strings[i]; *p; ++p) {
            if (*p == '(') {
                left_paren = p;
            } else if (*p == '+') {
                plus = p;
            } else if (*p == ')') {
                right_paren = p;
                break;
            }
        }

        char* name = strings[i];

        if (left_paren && right_paren && plus && left_paren + 1 < plus) {
            size_t new_mangled_size = size_t(plus - left_paren);

            if (mangled_size < new_mangled_size) {
                if (char* new_mangled = (char*)realloc(mangled, new_mangled_size)) {
                    mangled = new_mangled;
                    mangled_size = new_mangled_size;
                }
            }

            if (mangled_size >= new_mangled_size) {
                memcpy(mangled, left_paren + 1, new_mangled_size - 1);
                mangled[new_mangled_size - 1] = '\0';

                int status = -1;
                demangled =
                    abi::__cxa_demangle(mangled, demangled, &demangled_size, &status);

                if (status == 0 && demangled) {
                    name = demangled;
                }
            }
        }

        fprintf(stderr, "# %s\n", name);
    }

    // We call free() only after we've printed backtrace, since free() may
    // abort() if the heap is corrupted.

    free(demangled);
    free(mangled);
    free(strings);
}

void print_backtrace_emergency() {
    void* array[MaxDepth] = {};
    int size = backtrace(array, MaxDepth);

    if (size <= 0) {
        print_emergency_message("No backtrace available\n");
    } else {
        print_emergency_message("Backtrace:\n");
        backtrace_symbols_fd(array, size, STDERR_FILENO);
    }
}

} // namespace core
} // namespace roc
