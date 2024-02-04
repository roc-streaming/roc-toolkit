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
#include "roc_core/stddefs.h"
#include "roc_packet/ilink_meter.h"

namespace roc {
namespace pipeline {

//! Sender-side metrics specific to one participant (remote receiver).
struct SenderParticipantMetrics {
    //! Link metrics.
    packet::LinkMetrics link;

    //! Latency metrics.
    audio::LatencyMetrics latency;

    SenderParticipantMetrics() {
    }
};

//! Sender-side metrics of the whole slot.
struct SenderSlotMetrics {
    //! Is slot configuration complete.
    bool is_complete;

    //! Number of receivers connected to sender slot.
    size_t num_participants;

    SenderSlotMetrics()
        : is_complete(false)
        , num_participants(0) {
    }
};

//! Receiver-side metrics specific to one participant (remote sender).
struct ReceiverParticipantMetrics {
    //! Link metrics.
    packet::LinkMetrics link;

    //! Latency metrics.
    audio::LatencyMetrics latency;

    ReceiverParticipantMetrics() {
    }
};

//! Receiver-side metrics of the whole slot.
struct ReceiverSlotMetrics {
    //! Number of senders connected to receiver slot.
    size_t num_participants;

    ReceiverSlotMetrics()
        : num_participants(0) {
    }
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_METRICS_H_
