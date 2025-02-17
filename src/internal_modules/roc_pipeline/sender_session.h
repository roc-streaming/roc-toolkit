/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/sender_session.h
//! @brief Sender session.

#ifndef ROC_PIPELINE_SENDER_SESSION_H_
#define ROC_PIPELINE_SENDER_SESSION_H_

#include "roc_address/socket_addr.h"
#include "roc_audio/channel_mapper_writer.h"
#include "roc_audio/feedback_monitor.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_encoder.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/packetizer.h"
#include "roc_audio/processor_map.h"
#include "roc_audio/resampler_writer.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/scoped_ptr.h"
#include "roc_dbgio/csv_dumper.h"
#include "roc_fec/block_writer.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/router.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/metrics.h"
#include "roc_pipeline/sender_endpoint.h"
#include "roc_rtcp/communicator.h"
#include "roc_rtcp/composer.h"
#include "roc_rtcp/iparticipant.h"
#include "roc_rtp/encoding_map.h"
#include "roc_rtp/identity.h"
#include "roc_rtp/sequencer.h"
#include "roc_rtp/timestamp_extractor.h"
#include "roc_status/status_code.h"

namespace roc {
namespace pipeline {

//! Sender session sub-pipeline.
//!
//! Contains:
//!  - a pipeline for processing audio frames from single sender and converting
//!    them into packets
class SenderSession : public core::NonCopyable<>,
                      private rtcp::IParticipant,
                      private audio::IFrameWriter {
public:
    //! Initialize.
    SenderSession(const SenderSinkConfig& sink_config,
                  audio::ProcessorMap& processor_map,
                  rtp::EncodingMap& encoding_map,
                  packet::PacketFactory& packet_factory,
                  audio::FrameFactory& frame_factory,
                  core::IArena& arena,
                  dbgio::CsvDumper* dumper);

    //! Check if the pipeline was successfully constructed.
    status::StatusCode init_status() const;

    //! Create transport sub-pipeline.
    ROC_ATTR_NODISCARD status::StatusCode
    create_transport_pipeline(SenderEndpoint* source_endpoint,
                              SenderEndpoint* repair_endpoint);

    //! Create control sub-pipeline.
    ROC_ATTR_NODISCARD status::StatusCode
    create_control_pipeline(SenderEndpoint* control_endpoint);

    //! Get frame writer.
    //! @remarks
    //!  This way samples reach the pipeline.
    //!  Most of the processing, like encoding packets, generating redundancy packets,
    //!  etc, happens during the write operation.
    audio::IFrameWriter* frame_writer();

    //! Refresh pipeline according to current time.
    //! @remarks
    //!  Should be invoked before reading each frame.
    //!  If there are no frames for a while, should be invoked no
    //!  later than the deadline returned via @p next_deadline.
    ROC_ATTR_NODISCARD status::StatusCode refresh(core::nanoseconds_t current_time,
                                                  core::nanoseconds_t& next_deadline);

    //! Route a packet to the session.
    //! @remarks
    //!  This way feedback packets from receiver reach sender pipeline.
    //!  Packets are stored inside internal pipeline queues, and then fetched
    //!  when frame are passed from frame_writer().
    ROC_ATTR_NODISCARD status::StatusCode route_packet(const packet::PacketPtr& packet,
                                                       core::nanoseconds_t current_time);

    //! Get slot metrics.
    //! @remarks
    //!  These metrics are for the whole slot.
    //!  For metrics for specific participant, see get_participant_metrics().
    void get_slot_metrics(SenderSlotMetrics& slot_metrics) const;

    //! Get metrics for remote participants.
    //! @remarks
    //!  On sender, all participants corresponds to a single SenderSession.
    //!  In case of unicast, there is only one participant (remote receiver),
    //!  but in case of multicast, multiple participants may be present.
    //! @note
    //!  @p party_metrics points to array of metrics structs, and @p party_count
    //!  defines number of array elements. Metrics are written to given array,
    //!  and @p party_count is updated of actual number of elements written.
    //!  If there is not enough space for all metrics, result is truncated.
    void get_participant_metrics(SenderParticipantMetrics* party_metrics,
                                 size_t* party_count) const;

private:
    // Implementation of rtcp::IParticipant interface.
    // These methods are invoked by rtcp::Communicator.
    virtual rtcp::ParticipantInfo participant_info();
    virtual void change_source_id();
    virtual bool has_send_stream();
    virtual rtcp::SendReport query_send_stream(core::nanoseconds_t report_time);
    virtual status::StatusCode notify_send_stream(packet::stream_source_t recv_source_id,
                                                  const rtcp::RecvReport& recv_report);

    // Implementation of audio::IFrameWriter.
    virtual status::StatusCode write(audio::Frame& frame);

    void start_feedback_monitor_();

    status::StatusCode route_control_packet_(const packet::PacketPtr& packet,
                                             core::nanoseconds_t current_time);

    core::IArena& arena_;

    const SenderSinkConfig sink_config_;

    audio::ProcessorMap& processor_map_;
    rtp::EncodingMap& encoding_map_;

    packet::PacketFactory& packet_factory_;
    audio::FrameFactory& frame_factory_;

    core::Optional<rtp::Identity> identity_;
    core::Optional<rtp::Sequencer> sequencer_;

    core::Optional<packet::Router> router_;

    core::Optional<packet::Interleaver> interleaver_;

    core::ScopedPtr<fec::IBlockEncoder> fec_encoder_;
    core::Optional<fec::BlockWriter> fec_writer_;

    core::Optional<rtp::TimestampExtractor> timestamp_extractor_;

    core::ScopedPtr<audio::IFrameEncoder> payload_encoder_;
    core::Optional<audio::Packetizer> packetizer_;
    core::Optional<audio::ChannelMapperWriter> channel_mapper_writer_;
    core::Optional<audio::ResamplerWriter> resampler_writer_;
    core::SharedPtr<audio::IResampler> resampler_;

    core::Optional<audio::FeedbackMonitor> feedback_monitor_;

    core::Optional<rtcp::Communicator> rtcp_communicator_;
    address::SocketAddr rtcp_outbound_addr_;

    audio::IFrameWriter* frame_writer_;

    dbgio::CsvDumper* dumper_;

    status::StatusCode init_status_;
    status::StatusCode fail_status_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_SESSION_H_
