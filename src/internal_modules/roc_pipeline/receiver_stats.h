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

#include "roc_audio/latency_monitor.h"

namespace roc {
namespace pipeline {

//! Per participant statistics.
struct SessionStats {
    //! Latency statistics.
    audio::LatencyMonitorStats latency;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_STATS_H_
