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

#include "roc_audio/depacketizer.h"
#include "roc_audio/latency_config.h"
#include "roc_core/stddefs.h"
#include "roc_packet/ilink_meter.h"
#include "roc_packet/units.h"

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
    //! Slot source ID.
    packet::stream_source_t source_id;

    //! Number of participants (remote receivers) connected to slot.
    size_t num_participants;

    //! Is slot configuration complete (all endpoints bound).
    bool is_complete;

    SenderSlotMetrics()
        : source_id(0)
        , num_participants(0)
        , is_complete(false) {
    }
};

//! Receiver-side metrics specific to one participant (remote sender).
struct ReceiverParticipantMetrics {
    //! Link metrics.
    packet::LinkMetrics link;

    //! Latency metrics.
    audio::LatencyMetrics latency;

    //! Depacketizer metrics.
    audio::DepacketizerMetrics depacketizer;

    ReceiverParticipantMetrics() {
    }
};

//! Receiver-side metrics of the whole slot.
struct ReceiverSlotMetrics {
    //! Slot source ID.
    packet::stream_source_t source_id;

    //! Number of participants (remote senders) connected to slot.
    size_t num_participants;

    ReceiverSlotMetrics()
        : source_id(0)
        , num_participants(0) {
    }
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_METRICS_H_
