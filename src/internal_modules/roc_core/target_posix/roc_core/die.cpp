/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/die.h"
#include "roc_core/atomic_ops.h"
#include "roc_core/backtrace.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

namespace roc {
namespace core {

namespace {

int is_dying = 0;

void safe_print(const char* str) {
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

} // namespace

void die_fast(int code) {
    _exit(code);
}

void die_gracefully(const char* message, bool full_backtrace) {
    int no = 0, yes = 1;

    if (AtomicOps::compare_exchange_seq_cst(is_dying, no, yes)) {
        safe_print("\nERROR: ");
        safe_print(message);
        safe_print("\n\n");

        if (full_backtrace) {
            print_backtrace_full();
        } else {
            print_backtrace_safe();
        }
    }

    signal(SIGABRT, SIG_DFL);
    abort();
}

} // namespace core
} // namespace roc
