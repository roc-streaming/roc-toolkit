/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_STATS_H_
#define ROC_STATS_H_

#include "stddef.h"

namespace roc {
namespace pipeline {

struct Stats {
    core::nanoseconds_t end_to_end_latency;
    core::nanoseconds_t local_latency;

    Stats(core::nanoseconds_t e2e, core::nanoseconds_t local)
        : end_to_end_latency(e2e)
        , local_latency(local) {}
};

} // namespace pipeline
} // namespace roc

#endif // ROC_STATS_H_
