/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/fast_random.h"

#include <benchmark/benchmark.h>

namespace roc {
namespace core {
namespace {

void BM_Random_Fast(benchmark::State& state) {
    uint32_t r = 0;
    while (state.KeepRunning()) {
        r = fast_random_range(r, (uint32_t)-1);
        benchmark::DoNotOptimize(r);
    }
}

BENCHMARK(BM_Random_Fast);

} // namespace
} // namespace core
} // namespace roc
