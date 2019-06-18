/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "roc_core/format_time.h"

namespace roc {
namespace core {

bool format_time(char* buf, size_t bufsz) {
    timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        return false;
    }

    tm t;
    if (!localtime_r(&tv.tv_sec, &t)) {
        return false;
    }

    size_t off = strftime(buf, bufsz, "%H:%M:%S", &t);
    if (off == 0) {
        return false;
    }

    if (off == bufsz) {
        return false;
    }

    int ret =
        snprintf(buf + off, bufsz - off, ".%03lu", (unsigned long)tv.tv_usec / 1000);
    if (ret <= 0 || (size_t)ret >= (bufsz - off)) {
        return false;
    }

    return true;
}

} // namespace core
} // namespace roc
