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
#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_decoder.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/latency_monitor.h"
#include "roc_audio/pcm_mapper_reader.h"
#include "roc_audio/plc_reader.h"
#include "roc_audio/processor_map.h"
#include "roc_audio/resampler_reader.h"
#include "roc_audio/watchdog.h"
#include "roc_core/iarena.h"
#include "roc_core/list_node.h"
#include "roc_core/optional.h"
#include "roc_core/ref_counted.h"
#include "roc_core/scoped_ptr.h"
#include "roc_dbgio/csv_dumper.h"
#include "roc_fec/block_reader.h"
#include "roc_fec/iblock_decoder.h"
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
#include "roc_rtp/link_meter.h"
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
                        public core::ListNode<ReceiverSession>,
                        private audio::IFrameReader {
public:
    //! Initialize.
    ReceiverSession(const ReceiverSessionConfig& session_config,
                    const ReceiverCommonConfig& common_config,
                    audio::ProcessorMap& processor_map,
                    rtp::EncodingMap& encoding_map,
                    packet::PacketFactory& packet_factory,
                    audio::FrameFactory& frame_factory,
                    core::IArena& arena,
                    dbgio::CsvDumper* dumper);

    //! Check if the pipeline was successfully constructed.
    status::StatusCode init_status() const;

    //! Get frame reader.
    //! @remarks
    //!  This way samples are fetched from the pipeline.
    //!  Most of the processing, like decoding packets, restoring losses, and adjust
    //!  clock, happens during the read operation.
    audio::IFrameReader& frame_reader();

    //! Refresh pipeline according to current time.
    //! @remarks
    //!  Should be invoked before reading each frame.
    //!  If there are no frames for a while, should be invoked no
    //!  later than the deadline returned via @p next_deadline.
    ROC_ATTR_NODISCARD status::StatusCode refresh(core::nanoseconds_t current_time,
                                                  core::nanoseconds_t& next_deadline);

    //! Adjust session clock to match consumer clock.
    //! @remarks
    //!  @p playback_time specified absolute time when first sample of last frame
    //!  retrieved from pipeline will be actually played on sink
    void reclock(core::nanoseconds_t playback_time);

    //! Route a packet to the session.
    //! @remarks
    //!  This way packets from sender reach receiver pipeline.
    //!  Packets are stored inside internal pipeline queues, and then fetched
    //!  when frame are requested from frame_reader().
    ROC_ATTR_NODISCARD status::StatusCode route_packet(const packet::PacketPtr& packet);

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
    ReceiverParticipantMetrics get_metrics() const;

private:
    // Implementation of audio::IFrameReader.
    virtual status::StatusCode read(audio::Frame& frame,
                                    packet::stream_timestamp_t duration,
                                    audio::FrameReadMode mode);

    audio::IFrameReader* frame_reader_;

    core::Optional<packet::Router> packet_router_;

    core::Optional<packet::SortedQueue> source_queue_;
    core::Optional<packet::SortedQueue> repair_queue_;

    core::Optional<rtp::LinkMeter> source_meter_;
    core::Optional<rtp::LinkMeter> repair_meter_;

    core::ScopedPtr<audio::IFrameDecoder> payload_decoder_;

    core::Optional<rtp::Filter> filter_;
    core::Optional<packet::DelayedReader> delayed_reader_;

    core::Optional<rtp::Parser> fec_parser_;
    core::ScopedPtr<fec::IBlockDecoder> fec_decoder_;
    core::Optional<fec::BlockReader> fec_reader_;
    core::Optional<rtp::Filter> fec_filter_;

    core::Optional<rtp::TimestampInjector> timestamp_injector_;

    core::Optional<audio::Depacketizer> depacketizer_;
    core::ScopedPtr<audio::IPlc> plc_;
    core::Optional<audio::PlcReader> plc_reader_;
    core::Optional<audio::Watchdog> watchdog_;
    core::Optional<audio::ChannelMapperReader> channel_mapper_reader_;
    core::SharedPtr<audio::IResampler> resampler_;
    core::Optional<audio::ResamplerReader> resampler_reader_;

    core::Optional<audio::LatencyMonitor> latency_monitor_;

    dbgio::CsvDumper* dumper_;

    status::StatusCode init_status_;
    status::StatusCode fail_status_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SESSION_H_
