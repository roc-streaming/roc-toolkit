/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/CommandLineArguments.h>
#include <CppUTest/CommandLineTestRunner.h>

#include "roc_core/colors.h"
#include "roc_core/crash.h"
#include "roc_core/exit.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/stddefs.h"

using namespace roc;

int main(int argc, const char** argv) {
    core::HeapAllocator::enable_panic_on_leak();

    core::CrashHandler crash_handler;

    /* Check whether "-t" option is set.
     * If yes then change it to "-v" and remember this in "more_verbose" variable.
     *
     * Reason of changing "-t" to "-v" is that we also want to instruct
     * CppUTest to give more verbose output
     */

    bool more_verbose = false;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0) {
            argv[i] = "-v";
            more_verbose = true;
            break;
        }
    }

    CommandLineArguments args(argc, argv);

    if (more_verbose) {
        core::Logger::instance().set_level(LogTrace);
    } else if (args.parse(NULL) && args.isVerbose()) {
        core::Logger::instance().set_level(LogDebug);
    } else {
        core::Logger::instance().set_level(LogNone);
    }

    core::Logger::instance().set_colors(core::colors_available() ? core::ColorsEnabled
                                                                 : core::ColorsDisabled);

    MemoryLeakWarningPlugin::turnOffNewDeleteOverloads();

    const int code = CommandLineTestRunner::RunAllTests(argc, argv);
    if (code != 0) {
        core::fast_exit(code);
    }

    return 0;
}
