/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/CommandLineArguments.h>
#include <CppUTest/CommandLineTestRunner.h>
#include <string.h>

#include "roc_core/colors.h"
#include "roc_core/crash.h"
#include "roc_core/exit.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"

using namespace roc;

int main(int argc, char** argv) {
    core::HeapAllocator::enable_panic_on_leak();

    core::CrashHandler crash_handler;

    /* Check wether "-vv" option is set.
     * If yes then change it to "-v" and remember this in "moreVerbose" variable.
    */
    
    /*
     * Reason of changing "-vv" to "-v" is that we also want to instruct 
     * CppUTest to give more verbose output
    */
    
    bool moreVerbose = false;
    for(int i=0; i<argc; i++){
        if(strcmp(argv[i], "-vv")==0){
            moreVerbose = true;
            argv[i][2] = '\0';
        }
    }

    CommandLineArguments args(argc, (const char**) argv);
    
    if(moreVerbose){
        roc::core::Logger::instance().set_level(roc::LogTrace);
    } else if (args.parse(NULL) && args.isVerbose()) {
        roc::core::Logger::instance().set_level(roc::LogDebug);
    } else {
        roc::core::Logger::instance().set_level(roc::LogNone);
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
