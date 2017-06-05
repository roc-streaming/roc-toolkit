/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <stdlib.h>

#include <cxxabi.h>
#include <execinfo.h>

#include "roc_core/panic.h"

namespace roc {
namespace core {

enum { MaxDepth = 128 };

void print_backtrace() {
    char* names[MaxDepth] = {};
    void* array[MaxDepth] = {};

    int size = backtrace(array, MaxDepth);

    if (size <= 0) {
        fprintf(stderr, "No backtrace available\n");
    } else {
        fprintf(stderr, "Traceback (most recent call last):\n");

        char** strings = backtrace_symbols(array, size);

        for (int i = size - 1; i >= 0; i--) {
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
         * abort() if heap is corrupted ;)
         */
        for (int i = 0; i < size; i++) {
            free(names[i]);
        }

        free(strings);
    }
}

} // namespace core
} // namespace roc
