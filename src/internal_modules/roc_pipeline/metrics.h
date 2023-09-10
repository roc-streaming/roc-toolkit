/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/metrics.h
//! @brief Pipeline metrics.

#ifndef ROC_PIPELINE_METRICS_H_
#define ROC_PIPELINE_METRICS_H_

#include "roc_audio/latency_monitor.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace pipeline {

//! Metrics of sender session (connection to receiver).
struct SenderSessionMetrics {
    SenderSessionMetrics() {
    }
};

//! Metrics of sender slot.
struct SenderSlotMetrics {
    //! Is slot configuration complete.
    bool is_complete;

    SenderSlotMetrics()
        : is_complete(false) {
    }
};

//! Metrics of receiver session (connection from sender).
struct ReceiverSessionMetrics {
    //! Latency metrics.
    audio::LatencyMonitorMetrics latency;
};

//! Metrics of receiver slot.
struct ReceiverSlotMetrics {
    //! Number of sessions connected to receiver.
    size_t num_sessions;

    ReceiverSlotMetrics()
        : num_sessions(0) {
    }
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_METRICS_H_
