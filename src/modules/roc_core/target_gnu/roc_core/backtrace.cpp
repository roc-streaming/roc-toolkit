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

enum { MaxDepth = 128 };

} // namespace

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
            char* begin = NULL;
            char* end = NULL;
            char* mangled = NULL;

            for (char* p = strings[i]; *p; ++p) {
                if (*p == '(') {
                    mangled = p;
                } else if (*p == '+') {
                    begin = p;
                } else if (*p == ')') {
                    end = p;
                    break;
                }
            }

            if (mangled && begin && end && mangled < begin) {
                *mangled++ = '\0';
                *begin++ = '\0';
                *end++ = '\0';
            }

            int status = -1;

            if (mangled) {
                names[i] = abi::__cxa_demangle(mangled, 0, 0, &status);
            }

            fprintf(stderr, "# %s\n", status == 0 ? names[i] : strings[i]);
        }

        /* Call free() only after we've printed backtrace, since free() may
         * abort() if heap is corrupted.
         */
        for (int i = 0; i < size; i++) {
            free(names[i]);
        }

        free(strings);
    }
}

void print_emergency_backtrace() {
    void* array[MaxDepth] = {};
    int size = backtrace(array, MaxDepth);

    if (size <= 0) {
        print_emergency_string("No backtrace available\n");
    } else {
        print_emergency_string("Backtrace:\n");
        backtrace_symbols_fd(array, size, STDERR_FILENO);
    }
}

void print_emergency_string(const char* str) {
    size_t str_sz = strlen(str);
    while (str_sz > 0) {
        ssize_t ret = write(STDERR_FILENO, str, str_sz);
        if (ret <= 0) {
            return;
        }
        str += (size_t)ret;
        str_sz -= (size_t)ret;
    }
}

} // namespace core
} // namespace roc
