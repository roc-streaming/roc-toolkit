/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "roc_core/format_tid.h"

namespace roc {
namespace core {

#if defined(SYS_gettid)
bool format_tid(char* buf, size_t bufsz) {
    pid_t tid = (pid_t)syscall(SYS_gettid);
    int ret = snprintf(buf, bufsz, "%llu", (unsigned long long)tid);
    return ret > 0 && (size_t)ret < bufsz;
}
#else  // !defined(SYS_gettid)
bool format_tid(char* buf, size_t bufsz) {
    pid_t tid = getpid();
    int ret = snprintf(buf, bufsz, "%llu", (unsigned long long)tid);
    return ret > 0 && (size_t)ret < bufsz;
}
#endif // defined(SYS_gettid)

} // namespace core
} // namespace roc
