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

#include "roc_audio/channel_mapper_writer.h"
#include "roc_audio/iframe_encoder.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/packetizer.h"
#include "roc_audio/resampler_writer.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/scoped_ptr.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_fec/writer.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/router.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/metrics.h"
#include "roc_pipeline/sender_endpoint.h"
#include "roc_rtcp/communicator.h"
#include "roc_rtcp/composer.h"
#include "roc_rtcp/istream_controller.h"
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
class SenderSession : public core::NonCopyable<>, private rtcp::IStreamController {
public:
    //! Initialize.
    SenderSession(const SenderConfig& config,
                  const rtp::EncodingMap& encoding_map,
                  packet::PacketFactory& packet_factory,
                  core::BufferFactory<uint8_t>& byte_buffer_factory,
                  core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                  core::IArena& arena);

    //! Check if the session was succefully constructed.
    bool is_valid() const;

    //! Create transport sub-pipeline.
    bool create_transport_pipeline(SenderEndpoint* source_endpoint,
                                   SenderEndpoint* repair_endpoint);

    //! Create control sub-pipeline.
    bool create_control_pipeline(SenderEndpoint* control_endpoint);

    //! Get frame writer.
    //! @remarks
    //!  This way samples reach the pipeline.
    //!  Most of the processing, like encoding packets, generating redundancy packets,
    //!  etc, happens during the write operation.
    audio::IFrameWriter* frame_writer() const;

    //! Route a packet to the session.
    //! @remarks
    //!  This way feedback packets from receiver reach sender pipeline.
    //!  Packets are stored inside internal pipeline queues, and then fetched
    //!  when frame are passed from frame_writer().
    ROC_ATTR_NODISCARD status::StatusCode route_packet(const packet::PacketPtr& packet,
                                                       core::nanoseconds_t current_time);

    //! Refresh pipeline according to current time.
    //! @returns
    //!  deadline (absolute time) when refresh should be invoked again
    //!  if there are no frames
    core::nanoseconds_t refresh(core::nanoseconds_t current_time);

    //! Get session metrics.
    SenderSessionMetrics get_metrics() const;

private:
    // Implementation of rtcp::IStreamController interface.
    // These methods are invoked by rtcp::Communicator.
    virtual const char* cname();
    virtual packet::stream_source_t source_id();
    virtual void change_source_id();
    virtual bool has_send_stream();
    virtual rtcp::SendReport query_send_stream(core::nanoseconds_t report_time);
    virtual status::StatusCode notify_send_stream(packet::stream_source_t recv_source_id,
                                                  const rtcp::RecvReport& recv_report);

    status::StatusCode route_control_packet_(const packet::PacketPtr& packet,
                                             core::nanoseconds_t current_time);

    core::IArena& arena_;

    const SenderConfig& config_;

    const rtp::EncodingMap& encoding_map_;

    packet::PacketFactory& packet_factory_;
    core::BufferFactory<uint8_t>& byte_buffer_factory_;
    core::BufferFactory<audio::sample_t>& sample_buffer_factory_;

    core::Optional<rtp::Identity> identity_;
    core::Optional<rtp::Sequencer> sequencer_;

    core::Optional<packet::Router> router_;

    core::Optional<packet::Interleaver> interleaver_;

    core::ScopedPtr<fec::IBlockEncoder> fec_encoder_;
    core::Optional<fec::Writer> fec_writer_;

    core::Optional<rtp::TimestampExtractor> timestamp_extractor_;

    core::ScopedPtr<audio::IFrameEncoder> payload_encoder_;
    core::Optional<audio::Packetizer> packetizer_;

    core::Optional<audio::ChannelMapperWriter> channel_mapper_writer_;

    core::Optional<audio::ResamplerWriter> resampler_writer_;
    core::SharedPtr<audio::IResampler> resampler_;

    core::Optional<rtcp::Composer> rtcp_composer_;
    core::Optional<rtcp::Communicator> rtcp_communicator_;

    audio::IFrameWriter* frame_writer_;

    bool valid_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_SESSION_H_
