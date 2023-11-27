/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_session_group.h
//! @brief Receiver session group.

#ifndef ROC_PIPELINE_RECEIVER_SESSION_GROUP_H_
#define ROC_PIPELINE_RECEIVER_SESSION_GROUP_H_

#include "roc_audio/mixer.h"
#include "roc_core/iarena.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/iwriter.h"
#include "roc_pipeline/metrics.h"
#include "roc_pipeline/receiver_session.h"
#include "roc_pipeline/receiver_state.h"
#include "roc_rtcp/communicator.h"
#include "roc_rtcp/composer.h"
#include "roc_rtcp/istream_controller.h"

namespace roc {
namespace pipeline {

//! Receiver session group.
//!
//! Contains:
//!  - a set of related receiver sessions
class ReceiverSessionGroup : public core::NonCopyable<>, private rtcp::IStreamController {
public:
    //! Initialize.
    ReceiverSessionGroup(const ReceiverConfig& receiver_config,
                         ReceiverState& receiver_state,
                         audio::Mixer& mixer,
                         const rtp::EncodingMap& encoding_map,
                         packet::PacketFactory& packet_factory,
                         core::BufferFactory<uint8_t>& byte_buffer_factory,
                         core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                         core::IArena& arena);

    ~ReceiverSessionGroup();

    //! Route packet to session.
    ROC_ATTR_NODISCARD status::StatusCode route_packet(const packet::PacketPtr& packet,
                                                       core::nanoseconds_t current_time);

    //! Refresh pipeline according to current time.
    //! @returns
    //!  deadline (absolute time) when refresh should be invoked again
    //!  if there are no frames
    core::nanoseconds_t refresh_sessions(core::nanoseconds_t current_time);

    //! Adjust session clock to match consumer clock.
    //! @remarks
    //!  @p playback_time specified absolute time when first sample of last frame
    //!  retrieved from pipeline will be actually played on sink
    void reclock_sessions(core::nanoseconds_t playback_time);

    //! Get number of alive sessions.
    size_t num_sessions() const;

    //! Get metrics for all sessions.
    //! @remarks
    //!  @p metrics defines array of metrics structs, and @p metrics_size
    //!  defines number of array elements. Metrics are written to given array,
    //!  and @p metrics_size is updated of actual number of elements written.
    //!  If there is not enough space for all sessions, result is truncated.
    void get_metrics(ReceiverSessionMetrics* metrics, size_t* metrics_size) const;

private:
    // Implementation of rtcp::IStreamController interface.
    // These methods are invoked by rtcp::Communicator.
    virtual const char* cname();
    virtual packet::stream_source_t source_id();
    virtual void change_source_id();
    virtual size_t num_recv_steams();
    virtual rtcp::RecvReport query_recv_stream(size_t recv_stream_index,
                                               core::nanoseconds_t report_time);
    virtual void notify_recv_stream(packet::stream_source_t send_source_id,
                                    const rtcp::SendReport& send_report);
    virtual void halt_recv_stream(packet::stream_source_t send_source_id);

    status::StatusCode route_transport_packet_(const packet::PacketPtr& packet);
    status::StatusCode route_control_packet_(const packet::PacketPtr& packet,
                                             core::nanoseconds_t current_time);

    bool can_create_session_(const packet::PacketPtr& packet);

    status::StatusCode create_session_(const packet::PacketPtr& packet);
    void remove_session_(ReceiverSession& sess);
    void remove_all_sessions_();

    ReceiverSessionConfig make_session_config_(const packet::PacketPtr& packet) const;

    core::IArena& arena_;

    packet::PacketFactory& packet_factory_;
    core::BufferFactory<uint8_t>& byte_buffer_factory_;
    core::BufferFactory<audio::sample_t>& sample_buffer_factory_;

    const rtp::EncodingMap& encoding_map_;

    audio::Mixer& mixer_;

    ReceiverState& receiver_state_;
    const ReceiverConfig& receiver_config_;

    core::Optional<rtcp::Composer> rtcp_composer_;
    core::Optional<rtcp::Communicator> rtcp_communicator_;

    core::List<ReceiverSession> sessions_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SESSION_GROUP_H_
