/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_session.h
//! @brief Receiver session pipeline.

#ifndef ROC_PIPELINE_RECEIVER_SESSION_H_
#define ROC_PIPELINE_RECEIVER_SESSION_H_

#include "roc_audio/depacketizer.h"
#include "roc_audio/idecoder.h"
#include "roc_audio/ireader.h"
#include "roc_audio/resampler.h"
#include "roc_audio/resampler_updater.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"
#include "roc_core/unique_ptr.h"
#include "roc_fec/idecoder.h"
#include "roc_fec/reader.h"
#include "roc_packet/address.h"
#include "roc_packet/delayed_reader.h"
#include "roc_packet/iparser.h"
#include "roc_packet/ireader.h"
#include "roc_packet/packet.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/router.h"
#include "roc_packet/sorted_queue.h"
#include "roc_packet/watchdog.h"
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
    ReceiverSession(const SessionConfig& config,
                    const packet::Address& src_address,
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

    const packet::Address src_address_;

    core::IAllocator& allocator_;

    audio::IReader* audio_reader_;

    core::UniquePtr<packet::Router> queue_router_;

    core::UniquePtr<packet::SortedQueue> source_queue_;
    core::UniquePtr<packet::SortedQueue> repair_queue_;

    core::UniquePtr<packet::DelayedReader> delayed_reader_;
    core::UniquePtr<rtp::Validator> validator_;
    core::UniquePtr<packet::Watchdog> watchdog_;

    core::UniquePtr<rtp::Parser> fec_parser_;
    core::UniquePtr<fec::IDecoder> fec_decoder_;
    core::UniquePtr<fec::Reader> fec_reader_;
    core::UniquePtr<rtp::Validator> fec_validator_;
    core::UniquePtr<packet::Watchdog> fec_watchdog_;

    core::UniquePtr<audio::IDecoder> decoder_;
    core::UniquePtr<audio::Depacketizer> depacketizer_;

    core::UniquePtr<audio::Resampler> resampler_;
    core::UniquePtr<audio::ResamplerUpdater> resampler_updater_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SESSION_H_
