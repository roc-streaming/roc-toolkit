/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_capi/log.h"

#include "roc_core/log.h"

#include <stdio.h>

static FILE *flog = NULL;

using namespace roc;

namespace {

void loger(LogLevel level, const char* module, const char* message)
{
    const char slevel[][16] = {
        "LOG_NONE",
        "LOG_ERROR",
        "LOG_DEBUG",
        "LOG_TRACE",
        "LOG_FLOOD"
    };

    if (!flog)
        return;

    fprintf(flog, "[%s]: %s: %s\n", slevel[level], module, message);
}

} // anonymous

void roc_log_set_level(const unsigned int verbosity)
{
    (void)verbosity;

    flog = fopen("/tmp/roc.log", "wa");

    core::set_log_handler(loger);
    core::set_log_level(LogLevel(LOG_FLOOD));
}
