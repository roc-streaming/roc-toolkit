/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/printer.h"
#include "roc_core/console.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

namespace {

void default_println_func(const char* buf, size_t bufsz) {
    console_println("%.*s", (int)bufsz, buf);
}

} // namespace

Printer::Printer(PrintlnFunc println_func)
    : println_(println_func ? println_func : default_println_func)
    , bufsz_(0) {
}

Printer::~Printer() {
    flush_(true);
}

size_t Printer::writef(const char* format, ...) {
    va_list args;
    va_start(args, format);

    const size_t avail_sz = sizeof(buf_) - bufsz_ - 1;
    int needed_sz = vsnprintf(buf_ + bufsz_, avail_sz + 1, format, args);

    va_end(args);

    if (needed_sz < 0) {
        roc_panic("printer: invalid format");
    }

    if (needed_sz > (int)avail_sz) {
        roc_panic("printer: overflow");
    }

    bufsz_ += (size_t)needed_sz;
    flush_(false);

    return (size_t)needed_sz;
}

void Printer::flush_(bool force) {
    if (bufsz_ == 0) {
        return;
    }

    const char* curr = buf_;

    for (;;) {
        const char* next = strchr(curr, '\n');

        if (!next && !force) {
            break;
        }
        if (!next) {
            next = buf_ + bufsz_;
        }

        println_(curr, size_t(next - curr));

        if (next == buf_ + bufsz_) {
            break;
        }
        curr = next + 1;
    }

    if (curr == buf_ + bufsz_) {
        bufsz_ = 0;
    } else {
        const size_t size = size_t(curr - buf_);
        memmove(buf_, buf_ + size, bufsz_ - size);
        bufsz_ -= size;
    }
}

} // namespace core
} // namespace roc
