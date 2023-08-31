/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_stats.h
//! @brief A struct of per-participant statistics.

#ifndef ROC_PIPELINE_RECEIVER_STATS_H_
#define ROC_PIPELINE_RECEIVER_STATS_H_

#include "stddef.h"

namespace roc {
namespace pipeline {

//! Per participant statistics.
struct Stats {
    //! Overall participant latency.
    //! How much time does it take for a just formed audio::Frame on a sender side
    //! to be played on a receiver.
    core::nanoseconds_t end_to_end_latency;

    //! Local latency.
    //! How much time does it take for a just arrived packet to be played.
    core::nanoseconds_t local_latency;

    //! Constructor.
    Stats(core::nanoseconds_t e2e, core::nanoseconds_t local)
        : end_to_end_latency(e2e)
        , local_latency(local) {
    }
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_STATS_H_
