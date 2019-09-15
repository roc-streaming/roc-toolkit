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

enum { MaxDepth = 128, MaxLen = 128 };

} // namespace

// This function tries to performs symbol demangling, which uses signal-unsafe
// functions and works only with -rdynamic option (enabled in debug builds).
void print_backtrace() {
    char* names[MaxDepth] = {};
    void* array[MaxDepth] = {};

    int size = backtrace(array, MaxDepth);

    if (size <= 0) {
        fprintf(stderr, "No backtrace available\n");
    } else {
        fprintf(stderr, "Backtrace:\n");

        char** strings = backtrace_symbols(array, size);

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

            if (left_paren && right_paren && plus && left_paren + 1 < plus) {
                size_t mangled_size = size_t(plus - left_paren - 1);

                if (mangled_size < MaxLen - 1) {
                    char mangled[MaxLen];
                    memcpy(mangled, left_paren + 1, mangled_size);
                    mangled[mangled_size] = '\0';

                    int status = -1;
                    char* demangled = abi::__cxa_demangle(mangled, 0, 0, &status);

                    if (status == 0) {
                        names[i] = demangled;
                    }
                }
            }

            fprintf(stderr, "# %s\n", names[i] ? names[i] : strings[i]);
        }

        // Call free() only after we've printed backtrace, since free() may
        // abort() if heap is corrupted.
        for (int i = 0; i < size; i++) {
            free(names[i]);
        }

        free(strings);
    }
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
