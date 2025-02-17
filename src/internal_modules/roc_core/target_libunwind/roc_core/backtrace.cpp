/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// Select optimized API without remote unwinding.
#define UNW_LOCAL_ONLY

// Workaround to support building with -std=c++98.
#ifdef ROC_ATTR_ALIGNED
#define alignas(x) ROC_ATTR_ALIGNED(x)
#endif

#include <libunwind.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "roc_core/attributes.h"
#include "roc_core/backtrace.h"
#include "roc_core/string_builder.h"

namespace roc {
namespace core {

namespace {

enum { MaxFunctionNameLen = 100, MaxLineLen = 200 };

/* Write string to stderr.
 * This must be signal safe.
 */
void backtrace_write_string(const char* str) {
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

/* Checking if backtrace is empty or not.
 * This must be signal safe.
 */
bool is_backtrace_available() {
    /* To store the snapshot of the CPU registers.
     */
    unw_context_t context;

    /* To get snapshot of the CPU register.
     * unw_getcontext() is signal safe.
     */
    if (unw_getcontext(&context) < 0) {
        return false;
    }

    /* To point to the current frame in the call stack.
     */
    unw_cursor_t cursor;

    /* To point to current frame.
     * unw_init_local() is signal safe.
     */
    if (unw_init_local(&cursor, &context) < 0) {
        return false;
    }

    /* Moving to previously called frames & going through each of them.
     * unw_step() is signal safe.
     * If there is at least one entry in backtrace
     */
    if (unw_step(&cursor) > 0) {
        return true;
    }
    return false;
}

/* Print function name, offset, instruction pointer address
 * This must be signal safe if enable_demangling is false.
 */
void backtrace_symbols(bool enable_demangling) {
    /* To store the snapshot of the CPU registers.
     */
    unw_context_t context;

    /* To get snapshot of the CPU register.
     * unw_getcontext() is signal safe.
     */
    unw_getcontext(&context);

    /* To point to the current frame in the call stack
     */
    unw_cursor_t cursor;

    /* To point to current frame.
     * unw_init_local() is signal safe.
     */
    unw_init_local(&cursor, &context);

    /* Buffer for demangling.
     */
    char* demangled_buf = NULL;
    size_t demangled_size = 0;

    /* Moving to previously called frames & going through each of them.
     * unw_step() is signal safe.
     * buffer = final output that will be written to stderr.
     */
    uint32_t index = 0;
    while (unw_step(&cursor) > 0) {
        index++;
        /* Get value stored in instruction register
         * unw_get_reg() is signal safe.
         */
        unw_word_t ip;
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        /* Get function name & offset
         * unw_get_proc_name() is signal safe.
         * 'status' checks if unw_get_proc_name() is successful or not.
         * 'status = 0', successfully executed the function.
         */
        char function_name[MaxFunctionNameLen];
        function_name[0] = '\0';
        unw_word_t offset;
        int32_t status =
            unw_get_proc_name(&cursor, function_name, sizeof(function_name), &offset);
        if (status < 0) {
            offset = 0;
        }

        /* Demangling is not signal-safe.
         */
        const char* symbol = NULL;
        if (enable_demangling) {
            symbol = demangle_symbol(function_name, demangled_buf, demangled_size);
        }
        if (!symbol) {
            symbol = function_name;
        }

        /* Printing => index, function name, offset, ip.
         * The functions below are signal-safe.
         */
        char buffer[MaxLineLen];

        StringBuilder b(buffer, sizeof(buffer) - 1);

        b.append_str("#");
        b.append_uint(index, 10);

        b.append_str(": 0x");
        b.append_uint(ip, 16);

        b.append_str(" ");
        b.append_str(symbol);

        b.append_str("+0x");
        b.append_uint(offset, 16);

        strcat(buffer, "\n");

        backtrace_write_string(buffer);
    }

    if (enable_demangling) {
        free(demangled_buf);
    }
}

} // namespace

void print_backtrace_full() {
    if (!is_backtrace_available()) {
        fprintf(stderr, "No backtrace available\n");
        fflush(stderr);
    } else {
        fprintf(stderr, "Backtrace:\n");
        fflush(stderr);

        backtrace_symbols(true);
    }
}

void print_backtrace_safe() {
    if (is_backtrace_available()) {
        backtrace_symbols(false);
    }
}

} // namespace core
} // namespace roc
