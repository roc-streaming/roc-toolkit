/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_slot.h
//! @brief Receiver slot.

#ifndef ROC_PIPELINE_RECEIVER_SLOT_H_
#define ROC_PIPELINE_RECEIVER_SLOT_H_

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/mixer.h"
#include "roc_audio/processor_map.h"
#include "roc_core/iarena.h"
#include "roc_core/list_node.h"
#include "roc_core/ref_counted.h"
#include "roc_dbgio/csv_dumper.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/metrics.h"
#include "roc_pipeline/receiver_endpoint.h"
#include "roc_pipeline/receiver_session_group.h"
#include "roc_pipeline/state_tracker.h"
#include "roc_rtp/encoding_map.h"

namespace roc {
namespace pipeline {

//! Receiver slot.
//!
//! Contains:
//!  - one or more related receiver endpoints, one per each type
//!  - one session group associated with those endpoints
class ReceiverSlot : public core::RefCounted<ReceiverSlot, core::ArenaAllocation>,
                     public core::ListNode<> {
public:
    //! Initialize.
    ReceiverSlot(const ReceiverSourceConfig& source_config,
                 const ReceiverSlotConfig& slot_config,
                 StateTracker& state_tracker,
                 audio::Mixer& mixer,
                 audio::ProcessorMap& processor_map,
                 rtp::EncodingMap& encoding_map,
                 packet::PacketFactory& packet_factory,
                 audio::FrameFactory& frame_factory,
                 core::IArena& arena,
                 dbgio::CsvDumper* dumper);

    //! Check if the pipeline was successfully constructed.
    status::StatusCode init_status() const;

    //! Add endpoint.
    ReceiverEndpoint* add_endpoint(address::Interface iface,
                                   address::Protocol proto,
                                   const address::SocketAddr& inbound_address,
                                   packet::IWriter* outbound_writer);

    //! Pull packets and refresh pipeline according to current time.
    //! @remarks
    //!  Should be invoked before reading each frame.
    //!  If there are no frames for a while, should be invoked no
    //!  later than the deadline returned via @p next_deadline.
    ROC_ATTR_NODISCARD status::StatusCode refresh(core::nanoseconds_t current_time,
                                                  core::nanoseconds_t& next_deadline);

    //! Adjust sessions clock to match consumer clock.
    //! @remarks
    //!  @p playback_time specified absolute time when first sample of last frame
    //!  retrieved from pipeline will be actually played on sink
    void reclock(core::nanoseconds_t playback_time);

    //! Get number of alive sessions.
    size_t num_sessions() const;

    //! Get metrics for slot and its participants.
    void get_metrics(ReceiverSlotMetrics& slot_metrics,
                     ReceiverParticipantMetrics* party_metrics,
                     size_t* party_count) const;

private:
    ReceiverEndpoint* create_source_endpoint_(address::Protocol proto,
                                              const address::SocketAddr& inbound_address,
                                              packet::IWriter* outbound_writer);
    ReceiverEndpoint* create_repair_endpoint_(address::Protocol proto,
                                              const address::SocketAddr& inbound_address,
                                              packet::IWriter* outbound_writer);
    ReceiverEndpoint* create_control_endpoint_(address::Protocol proto,
                                               const address::SocketAddr& inbound_address,
                                               packet::IWriter* outbound_writer);

    rtp::EncodingMap& encoding_map_;

    StateTracker& state_tracker_;
    ReceiverSessionGroup session_group_;

    core::Optional<ReceiverEndpoint> source_endpoint_;
    core::Optional<ReceiverEndpoint> repair_endpoint_;
    core::Optional<ReceiverEndpoint> control_endpoint_;

    status::StatusCode init_status_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SLOT_H_
