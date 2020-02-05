/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/sender_endpoint_set.h
//! @brief Sender endpoint set.

#ifndef ROC_PIPELINE_SENDER_ENDPOINT_SET_H_
#define ROC_PIPELINE_SENDER_ENDPOINT_SET_H_

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_audio/iframe_encoder.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/packetizer.h"
#include "roc_audio/poison_writer.h"
#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_writer.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/refcnt.h"
#include "roc_core/scoped_ptr.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_fec/writer.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/router.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/sender_endpoint.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {

//! Sender endpoint set.
//! @remarks
//!  Contains one or seevral related endpoint pipelines and
//!  the part of the sender pipeline shared by them.
class SenderEndpointSet : public core::RefCnt<SenderEndpointSet>, public core::ListNode {
public:
    //! Initialize.
    SenderEndpointSet(const SenderConfig& config,
                      const rtp::FormatMap& format_map,
                      packet::PacketPool& packet_pool,
                      core::BufferPool<uint8_t>& byte_buffer_pool,
                      core::BufferPool<audio::sample_t>& sample_buffer_pool,
                      core::IAllocator& allocator);

    //! Add endpoint.
    SenderEndpoint* add_endpoint(address::Interface iface, address::Protocol proto);

    //! Get audio writer.
    //! @returns NULL if endpoint set is not ready.
    audio::IWriter* writer();

    //! Check if endpoint set configuration is done.
    bool is_ready() const;

private:
    friend class core::RefCnt<SenderEndpointSet>;

    void destroy();

    SenderEndpoint* create_source_endpoint_(address::Protocol proto);
    SenderEndpoint* create_repair_endpoint_(address::Protocol proto);

    bool create_pipeline_();

    const SenderConfig& config_;

    const rtp::FormatMap& format_map_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& byte_buffer_pool_;
    core::BufferPool<audio::sample_t>& sample_buffer_pool_;

    core::IAllocator& allocator_;

    core::ScopedPtr<SenderEndpoint> source_endpoint_;
    core::ScopedPtr<SenderEndpoint> repair_endpoint_;

    core::ScopedPtr<packet::Router> router_;

    core::ScopedPtr<packet::Interleaver> interleaver_;

    core::ScopedPtr<fec::IBlockEncoder> fec_encoder_;
    core::ScopedPtr<fec::Writer> fec_writer_;

    core::ScopedPtr<audio::IFrameEncoder> payload_encoder_;
    core::ScopedPtr<audio::Packetizer> packetizer_;

    core::ScopedPtr<audio::PoisonWriter> resampler_poisoner_;
    core::ScopedPtr<audio::ResamplerWriter> resampler_writer_;
    core::ScopedPtr<audio::IResampler> resampler_;

    audio::IWriter* audio_writer_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_ENDPOINT_SET_H_
