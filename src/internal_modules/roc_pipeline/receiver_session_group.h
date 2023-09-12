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
#include "roc_pipeline/metrics.h"
#include "roc_pipeline/receiver_session.h"
#include "roc_pipeline/receiver_state.h"
#include "roc_rtcp/composer.h"
#include "roc_rtcp/session.h"

namespace roc {
namespace pipeline {

//! Receiver session group.
//!
//! Contains:
//!  - a set of related receiver sessions
class ReceiverSessionGroup : public core::NonCopyable<>, private rtcp::IReceiverHooks {
public:
    //! Initialize.
    ReceiverSessionGroup(const ReceiverConfig& receiver_config,
                         ReceiverState& receiver_state,
                         audio::Mixer& mixer,
                         const rtp::FormatMap& format_map,
                         packet::PacketFactory& packet_factory,
                         core::BufferFactory<uint8_t>& byte_buffer_factory,
                         core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                         core::IArena& arena);

    ~ReceiverSessionGroup();

    //! Route packet to session.
    void route_packet(const packet::PacketPtr& packet);

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
    // Implementation of rtcp::IReceiverHooks interface.
    // These methods are invoked by rtcp::Session.
    virtual void on_update_source(packet::source_t ssrc, const char* cname);
    virtual void on_remove_source(packet::source_t ssrc);
    virtual size_t on_get_num_sources();
    virtual rtcp::ReceptionMetrics on_get_reception_metrics(size_t source_index);
    virtual void on_add_sending_metrics(const rtcp::SendingMetrics& metrics);
    virtual void on_add_link_metrics(const rtcp::LinkMetrics& metrics);

    void route_transport_packet_(const packet::PacketPtr& packet);
    void route_control_packet_(const packet::PacketPtr& packet);

    bool can_create_session_(const packet::PacketPtr& packet);

    void create_session_(const packet::PacketPtr& packet);
    void remove_session_(ReceiverSession& sess);
    void remove_all_sessions_();

    ReceiverSessionConfig make_session_config_(const packet::PacketPtr& packet) const;

    core::IArena& arena_;

    packet::PacketFactory& packet_factory_;
    core::BufferFactory<uint8_t>& byte_buffer_factory_;
    core::BufferFactory<audio::sample_t>& sample_buffer_factory_;

    const rtp::FormatMap& format_map_;

    audio::Mixer& mixer_;

    ReceiverState& receiver_state_;
    const ReceiverConfig& receiver_config_;

    core::Optional<rtcp::Composer> rtcp_composer_;
    core::Optional<rtcp::Session> rtcp_session_;

    core::List<ReceiverSession> sessions_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SESSION_GROUP_H_
