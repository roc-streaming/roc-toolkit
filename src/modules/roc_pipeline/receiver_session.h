/*
 * Copyright (c) 2017 Roc authors
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
#include "roc_audio/depacketizer.h"
#include "roc_audio/iframe_decoder.h"
#include "roc_audio/ireader.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/latency_monitor.h"
#include "roc_audio/poison_reader.h"
#include "roc_audio/resampler_reader.h"
#include "roc_audio/watchdog.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"
#include "roc_core/unique_ptr.h"
#include "roc_fec/codec_map.h"
#include "roc_fec/iblock_decoder.h"
#include "roc_fec/reader.h"
#include "roc_packet/delayed_reader.h"
#include "roc_packet/iparser.h"
#include "roc_packet/ireader.h"
#include "roc_packet/packet.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/router.h"
#include "roc_packet/sorted_queue.h"
#include "roc_pipeline/config.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/parser.h"
#include "roc_rtp/validator.h"

namespace roc {
namespace pipeline {

//! Receiver session pipeline.
//! @remarks
//!  Created at the receiver side for every connected sender.
class ReceiverSession : public core::RefCnt<ReceiverSession>, public core::ListNode {
public:
    //! Initialize.
    ReceiverSession(const ReceiverSessionConfig& session_config,
                    const ReceiverCommonConfig& common_config,
                    const address::SocketAddr& src_address,
                    const fec::CodecMap& codec_map,
                    const rtp::FormatMap& format_map,
                    packet::PacketPool& packet_pool,
                    core::BufferPool<uint8_t>& byte_buffer_pool,
                    core::BufferPool<audio::sample_t>& sample_buffer_pool,
                    core::IAllocator& allocator);

    //! Check if the session pipeline was succefully constructed.
    bool valid() const;

    //! Try to route a packet to this session.
    //! @returns
    //!  true if the packet is dedicated for this session
    bool handle(const packet::PacketPtr& packet);

    //! Update session.
    //! @returns
    //!  false if the session is terminated
    bool update(packet::timestamp_t time);

    //! Get audio reader.
    audio::IReader& reader();

private:
    friend class core::RefCnt<ReceiverSession>;

    void destroy();

    const address::SocketAddr src_address_;

    core::IAllocator& allocator_;

    audio::IReader* audio_reader_;

    core::UniquePtr<packet::Router> queue_router_;

    core::UniquePtr<packet::SortedQueue> source_queue_;
    core::UniquePtr<packet::SortedQueue> repair_queue_;

    core::UniquePtr<packet::DelayedReader> delayed_reader_;
    core::UniquePtr<rtp::Validator> validator_;
    core::UniquePtr<audio::Watchdog> watchdog_;

    core::UniquePtr<rtp::Parser> fec_parser_;
    core::UniquePtr<fec::IBlockDecoder> fec_decoder_;
    core::UniquePtr<fec::Reader> fec_reader_;
    core::UniquePtr<rtp::Validator> fec_validator_;

    core::UniquePtr<audio::IFrameDecoder> payload_decoder_;
    core::UniquePtr<audio::Depacketizer> depacketizer_;

    core::UniquePtr<audio::PoisonReader> resampler_poisoner_;
    core::UniquePtr<audio::ResamplerReader> resampler_reader;
    core::UniquePtr<audio::IResampler> resampler_;

    core::UniquePtr<audio::PoisonReader> session_poisoner_;

    core::UniquePtr<audio::LatencyMonitor> latency_monitor_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SESSION_H_
