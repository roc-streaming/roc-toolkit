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

using namespace roc;

int main(int argc, const char** argv) {
    core::HeapAllocator::enable_panic_on_leak();

    core::CrashHandler crash_handler;

    CommandLineArguments args(argc, argv);

    if (args.parse(NULL) && args.isVerbose()) {
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
