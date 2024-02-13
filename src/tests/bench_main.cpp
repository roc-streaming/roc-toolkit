/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <benchmark/benchmark.h>

#include "roc_core/crash_handler.h"
#include "roc_core/heap_arena.h"
#include "roc_core/log.h"

using namespace roc;

int main(int argc, char** argv) {
    core::HeapArena::set_guards(core::HeapArena_DefaultGuards
                                | core::HeapArena_LeakGuard);

    core::CrashHandler crash_handler;

    core::Logger::instance().set_level(LogError);
    core::Logger::instance().set_colors(core::ColorsAuto);

    benchmark::Initialize(&argc, argv);

    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }

    benchmark::RunSpecifiedBenchmarks();

    return 0;
}
