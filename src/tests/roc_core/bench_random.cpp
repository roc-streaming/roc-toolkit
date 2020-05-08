/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <benchmark/benchmark.h>

#include "roc_core/fast_random.h"
#include "roc_core/secure_random.h"

namespace roc {
namespace core {
namespace {

void BM_Random_Fast(benchmark::State& state) {
    uint32_t r = 0;
    while (state.KeepRunning()) {
        r = fast_random(r, (uint32_t)-1);
        benchmark::DoNotOptimize(r);
    }
}

BENCHMARK(BM_Random_Fast);

void BM_Random_Secure(benchmark::State& state) {
    uint32_t r = 0;
    while (state.KeepRunning()) {
        secure_random(r, (uint32_t)-1, r);
        benchmark::DoNotOptimize(r);
    }
}

BENCHMARK(BM_Random_Secure);

} // namespace
} // namespace core
} // namespace roc
