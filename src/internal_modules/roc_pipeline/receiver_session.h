/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_session.h
//! @brief Receiver session pipeline.

#ifndef ROC_PIPELINE_RECEIVER_SESSION_H_
#define ROC_PIPELINE_RECEIVER_SESSION_H_

#include "roc_address/socket_addr.h"
#include "roc_audio/channel_mapper_reader.h"
#include "roc_audio/depacketizer.h"
#include "roc_audio/iframe_decoder.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/latency_monitor.h"
#include "roc_audio/poison_reader.h"
#include "roc_audio/resampler_reader.h"
#include "roc_audio/watchdog.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/iarena.h"
#include "roc_core/list_node.h"
#include "roc_core/optional.h"
#include "roc_core/ref_counted.h"
#include "roc_core/scoped_ptr.h"
#include "roc_fec/iblock_decoder.h"
#include "roc_fec/reader.h"
#include "roc_packet/delayed_reader.h"
#include "roc_packet/iparser.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/router.h"
#include "roc_packet/sorted_queue.h"
#include "roc_packet/units.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/metrics.h"
#include "roc_rtcp/reports.h"
#include "roc_rtp/encoding_map.h"
#include "roc_rtp/filter.h"
#include "roc_rtp/parser.h"
#include "roc_rtp/timestamp_injector.h"

namespace roc {
namespace pipeline {

//! Receiver session sub-pipeline.
//!
//! Contains:
//!  - a pipeline for processing packets from single sender and converting
//!    them into audio frames
class ReceiverSession : public core::RefCounted<ReceiverSession, core::ArenaAllocation>,
                        public core::ListNode {
public:
    //! Initialize.
    ReceiverSession(const ReceiverSessionConfig& session_config,
                    const ReceiverCommonConfig& common_config,
                    const rtp::EncodingMap& encoding_map,
                    packet::PacketFactory& packet_factory,
                    core::BufferFactory<uint8_t>& byte_buffer_factory,
                    core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                    core::IArena& arena);

    //! Check if the session was succefully constructed.
    bool is_valid() const;

    //! Route a packet to this session.
    ROC_ATTR_NODISCARD status::StatusCode route_packet(const packet::PacketPtr& packet);

    //! Refresh pipeline according to current time.
    //! @remarks
    //!  writes to @p next_refresh deadline (absolute time) when refresh should
    //!  be invoked again if there are no frames
    //! @returns
    //!  false if the session is ended
    bool refresh(core::nanoseconds_t current_time, core::nanoseconds_t* next_refresh);

    //! Adjust session clock to match consumer clock.
    //! @remarks
    //!  @p playback_time specified absolute time when first sample of last frame
    //!  retrieved from pipeline will be actually played on sink
    //! @returns
    //!  false if the session is ended
    bool reclock(core::nanoseconds_t playback_time);

    //! Get number of RTCP reports to be generated.
    size_t num_reports() const;

    //! Generate RTCP reports to be delivered to sender.
    void generate_reports(const char* report_cname,
                          packet::stream_source_t report_ssrc,
                          core::nanoseconds_t report_time,
                          rtcp::RecvReport* reports,
                          size_t n_reports) const;

    //! Process RTCP report obtained from sender.
    void process_report(const rtcp::SendReport& report);

    //! Get session metrics.
    ReceiverSessionMetrics get_metrics() const;

    //! Get audio reader.
    audio::IFrameReader& reader();

private:
    audio::IFrameReader* audio_reader_;

    core::Optional<packet::Router> packet_router_;

    core::Optional<packet::SortedQueue> source_queue_;
    core::Optional<packet::SortedQueue> repair_queue_;

    core::ScopedPtr<audio::IFrameDecoder> payload_decoder_;

    core::Optional<rtp::Filter> filter_;
    core::Optional<packet::DelayedReader> delayed_reader_;
    core::Optional<audio::Watchdog> watchdog_;

    core::Optional<rtp::Parser> fec_parser_;
    core::ScopedPtr<fec::IBlockDecoder> fec_decoder_;
    core::Optional<fec::Reader> fec_reader_;
    core::Optional<rtp::Filter> fec_filter_;

    core::Optional<rtp::TimestampInjector> timestamp_injector_;

    core::Optional<audio::Depacketizer> depacketizer_;

    core::Optional<audio::ChannelMapperReader> channel_mapper_reader_;

    core::Optional<audio::PoisonReader> resampler_poisoner_;
    core::Optional<audio::ResamplerReader> resampler_reader_;
    core::SharedPtr<audio::IResampler> resampler_;

    core::Optional<audio::PoisonReader> session_poisoner_;

    core::Optional<audio::LatencyMonitor> latency_monitor_;

    bool valid_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SESSION_H_
