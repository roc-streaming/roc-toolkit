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
#include "roc_audio/resampler_map.h"
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
#include "roc_rtcp/composer.h"
#include "roc_rtcp/session.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/timestamp_extractor.h"

namespace roc {
namespace pipeline {

//! Sender session sub-pipeline.
//!
//! Contains:
//!  - a pipeline for processing audio frames from single sender and converting
//!    them into packets
class SenderSession : public core::NonCopyable<>, private rtcp::ISenderHooks {
public:
    //! Initialize.
    SenderSession(const SenderConfig& config,
                  const rtp::FormatMap& format_map,
                  packet::PacketFactory& packet_factory,
                  core::BufferFactory<uint8_t>& byte_buffer_factory,
                  core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                  core::IArena& arena);

    //! Create transport sub-pipeline.
    bool create_transport_pipeline(SenderEndpoint* source_endpoint,
                                   SenderEndpoint* repair_endpoint);

    //! Create control sub-pipeline.
    bool create_control_pipeline(SenderEndpoint* control_endpoint);

    //! Get audio writer.
    audio::IFrameWriter* writer() const;

    //! Refresh pipeline according to current time.
    //! @returns
    //!  deadline (absolute time) when refresh should be invoked again
    //!  if there are no frames
    core::nanoseconds_t refresh(core::nanoseconds_t current_time);

    //! Get session metrics.
    SenderSessionMetrics get_metrics() const;

private:
    // Implementation of rtcp::ISenderHooks interface.
    // These methods are invoked by rtcp::Session.
    virtual size_t on_get_num_sources();
    virtual packet::stream_source_t on_get_sending_source(size_t source_index);
    virtual rtcp::SendingMetrics on_get_sending_metrics(core::nanoseconds_t report_time);
    virtual void on_add_reception_metrics(const rtcp::ReceptionMetrics& metrics);
    virtual void on_add_link_metrics(const rtcp::LinkMetrics& metrics);

    core::IArena& arena_;

    const SenderConfig& config_;

    const rtp::FormatMap& format_map_;

    packet::PacketFactory& packet_factory_;
    core::BufferFactory<uint8_t>& byte_buffer_factory_;
    core::BufferFactory<audio::sample_t>& sample_buffer_factory_;

    core::Optional<packet::Router> router_;

    core::Optional<packet::Interleaver> interleaver_;

    core::ScopedPtr<fec::IBlockEncoder> fec_encoder_;
    core::Optional<fec::Writer> fec_writer_;

    core::Optional<rtp::TimestampExtractor> timestamp_extractor_;

    core::ScopedPtr<audio::IFrameEncoder> payload_encoder_;
    core::Optional<audio::Packetizer> packetizer_;

    core::Optional<audio::ChannelMapperWriter> channel_mapper_writer_;

    core::Optional<audio::ResamplerWriter> resampler_writer_;
    core::ScopedPtr<audio::IResampler> resampler_;

    core::Optional<rtcp::Composer> rtcp_composer_;
    core::Optional<rtcp::Session> rtcp_session_;

    audio::IFrameWriter* audio_writer_;

    size_t num_sources_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_SESSION_H_
