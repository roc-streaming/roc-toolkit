/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define UNW_LOCAL_ONLY
#include <libunwind.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "roc_core/backtrace.h"

/* Function to convert unsigned int to decimal/hex string.
 * This function is signal safe.
 * Parameters :
 * number = the number to be converted.
 * base = 10 for decimal to char* and 16 for hex to char* conversion.
 * output_string = the decimal/hex string
 */

namespace {

enum { MaxDigits = 10, MaxLen = 200 };

void uint32_to_string(uint32_t number, uint32_t base, char* output_string) {
    int i = MaxDigits;
    int j = 0;
    do {
        output_string[i] = "0123456789abcdef"[number % base];
        i--;
        number = number / base;
    } while (number > 0);

    while (++i < MaxDigits + 1)
        output_string[j++] = output_string[i];
    output_string[j] = 0;
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
    if (unw_getcontext(&context) < 0)
        return false;

    /* To point to the current frame in the call stack.
 */
    unw_cursor_t cursor;

    /* To point to current frame.
 * unw_init_local() is signal safe.
 */
    if (unw_init_local(&cursor, &context) < 0)
        return false;

    /* Moving to previously called frames & going through each of them.
     * unw_step() is signal safe.
     * If there is atleast one entry in backtrace
     */
    if (unw_step(&cursor) > 0)
        return true;
    return false;
}

/* Print function name, offset, instruction pointer address
 * This must be signal safe.
 */
void backtrace_symbols_fd(int fd) {
    /* To store the snapshot of the CPU registers.
     */
    unw_context_t context;

    /* To get snapshot of the CPU register.
     * unw_getcontext() is signal safe.
     */
    unw_getcontext(&context);

    /* To point to the current frame in the call stack.
 */
    unw_cursor_t cursor;

    /* To point to current frame.
 * unw_init_local() is signal safe.
 */
    unw_init_local(&cursor, &context);

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
        char function_name[128];
        function_name[0] = '\0';
        unw_word_t offset;
        int32_t status =
            unw_get_proc_name(&cursor, function_name, sizeof(function_name), &offset);
        /* Printing => index, function name, offset, ip
         * using write().
         * write() & strcat() are signal safe.
         */
        if (status < 0) {
            offset = 0;
        }
        char number[MaxDigits + 1];
        uint32_to_string(index, 10, number);

        char buffer[MaxLen] = "#";
        strcat(buffer, number);
        strcat(buffer, ": (");
        strcat(buffer, function_name);
        strcat(buffer, "+0x");

        uint32_to_string((uint32_t)offset, 16, number);
        strcat(buffer, number);
        strcat(buffer, ") [");

        uint32_to_string((uint32_t)ip, 16, number);
        strcat(buffer, number);
        strcat(buffer, "]\n");

        write(fd, buffer, 200);
    }
}
}

namespace roc {
namespace core {

void print_backtrace() {
    if (!is_backtrace_available()) {
        fprintf(stderr, "No backtrace available\n");
    } else {
        fprintf(stderr, "Backtrace:\n");
        backtrace_symbols_fd(STDERR_FILENO);
    }
}

void print_backtrace_emergency() {
    if (is_backtrace_available()) {
        backtrace_symbols_fd(STDERR_FILENO);
    }
}

} // namespace core
} // namespace roc
