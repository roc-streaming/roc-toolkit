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

#include "roc_audio/latency_tuner.h"
#include "roc_audio/packetizer.h"
#include "roc_core/stddefs.h"
#include "roc_rtp/link_meter.h"

namespace roc {
namespace pipeline {

//! Metrics of sender session (connection to receiver).
struct SenderSessionMetrics {
    //! Packetization metrics.
    audio::PacketizerMetrics packets;

    //! Receiver feedback.
    audio::LatencyMetrics latency;

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
    //! Link metrics.
    rtp::LinkMetrics link;

    //! Latency metrics.
    audio::LatencyMetrics latency;

    ReceiverSessionMetrics() {
    }
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
