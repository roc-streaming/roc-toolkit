/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/sender_port_group.h
//! @brief Sender port group.

#ifndef ROC_PIPELINE_SENDER_PORT_GROUP_H_
#define ROC_PIPELINE_SENDER_PORT_GROUP_H_

#include "roc_address/endpoint_type.h"
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
#include "roc_fec/codec_map.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_fec/writer.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/router.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/sender_port.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {

//! Sender port group.
class SenderPortGroup : public core::RefCnt<SenderPortGroup>, public core::ListNode {
public:
    //! Initialize.
    SenderPortGroup(const SenderConfig& config,
                    const fec::CodecMap& codec_map,
                    const rtp::FormatMap& format_map,
                    packet::PacketPool& packet_pool,
                    core::BufferPool<uint8_t>& byte_buffer_pool,
                    core::BufferPool<audio::sample_t>& sample_buffer_pool,
                    core::IAllocator& allocator);

    //! Add port.
    //! @returns false on error.
    SenderPort* add_port(address::EndpointType type,
                         const pipeline::PortConfig& port_config);

    //! Get audio writer.
    //! @returns null if it's pipeline is not fully configured yet.
    audio::IWriter* writer();

    //! Check if port group is fully configured.
    bool is_configured() const;

private:
    friend class core::RefCnt<SenderPortGroup>;

    void destroy();

    SenderPort* create_source_port_(const PortConfig& port_config);
    SenderPort* create_repair_port_(const PortConfig& port_config);

    bool create_pipeline_();

    const SenderConfig& config_;

    const fec::CodecMap& codec_map_;
    const rtp::FormatMap& format_map_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& byte_buffer_pool_;
    core::BufferPool<audio::sample_t>& sample_buffer_pool_;

    core::IAllocator& allocator_;

    core::ScopedPtr<SenderPort> source_port_;
    core::ScopedPtr<SenderPort> repair_port_;

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

#endif // ROC_PIPELINE_SENDER_PORT_GROUP_H_
