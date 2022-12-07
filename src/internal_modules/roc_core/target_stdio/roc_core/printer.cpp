/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "roc_core/panic.h"
#include "roc_core/printer.h"

namespace roc {
namespace core {

namespace {

void print_stderr(const char* buf, size_t bufsz) {
    fwrite(buf, 1, bufsz, stderr);
    fflush(stderr);
}

} // namespace

Printer::Printer(PrintFunc print_func)
    : print_(print_func ? print_func : &print_stderr) {
    buf_[0] = '\0';
    bufsz_ = 0;
}

Printer::~Printer() {
    flush_(true);
}

size_t Printer::writef(const char* format, ...) {
    for (;;) {
        va_list args;
        va_start(args, format);

        // available bytes, excluding terminator
        const size_t avail_sz = sizeof(buf_) - bufsz_ - 1;

        // needed bytes, excluding terminator
        int needed_sz = vsnprintf(buf_ + bufsz_, avail_sz + 1, format, args);

        va_end(args);

        if (needed_sz > (int)avail_sz && bufsz_ != 0) {
            buf_[bufsz_] = '\0';

            flush_(true);
            roc_panic_if(bufsz_ != 0);

            continue;
        }

        if (needed_sz > (int)avail_sz) {
            needed_sz = (int)avail_sz;
        }

        if (needed_sz >= 0) {
            bufsz_ += (size_t)needed_sz;
        }

        roc_panic_if(bufsz_ >= sizeof(buf_));

        buf_[bufsz_] = '\0';

        flush_(false);

        return needed_sz > 0 ? (size_t)needed_sz : 0;
    }
}

void Printer::flush() {
    flush_(true);
}

void Printer::flush_(bool force) {
    if (bufsz_ == 0) {
        return;
    }

    if (!force && bufsz_ < FlushThreshold) {
        return;
    }

    const char* last_nl = strrchr(buf_, '\n');
    size_t flush_size = 0;

    if (force || last_nl == NULL) {
        flush_size = bufsz_;
    } else {
        flush_size = size_t(last_nl - buf_) + 1;
    }

    roc_panic_if(flush_size > bufsz_);

    print_(buf_, flush_size);

    if (flush_size != bufsz_) {
        memmove(buf_, buf_ + flush_size, bufsz_ - flush_size);
    }

    bufsz_ -= flush_size;
    buf_[bufsz_] = '\0';
}

} // namespace core
} // namespace roc
