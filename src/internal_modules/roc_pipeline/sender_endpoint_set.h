/*
 * Copyright (c) 2020 Roc Streaming authors
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
#include "roc_core/buffer_factory.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/ref_counter.h"
#include "roc_core/scoped_ptr.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_fec/writer.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_factory.h"
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
class SenderEndpointSet : public core::RefCounter<SenderEndpointSet>,
                          public core::ListNode {
public:
    //! Initialize.
    SenderEndpointSet(const SenderConfig& config,
                      const rtp::FormatMap& format_map,
                      packet::PacketFactory& packet_factory,
                      core::BufferFactory<uint8_t>& byte_buffer_factory,
                      core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                      core::IAllocator& allocator);

    //! Add endpoint.
    SenderEndpoint* create_endpoint(address::Interface iface, address::Protocol proto);

    //! Get audio writer.
    //! @returns NULL if endpoint set is not ready.
    audio::IWriter* writer();

    //! Check if endpoint set configuration is done.
    bool is_ready() const;

private:
    friend class core::RefCounter<SenderEndpointSet>;

    void destroy();

    SenderEndpoint* create_source_endpoint_(address::Protocol proto);
    SenderEndpoint* create_repair_endpoint_(address::Protocol proto);

    bool create_pipeline_();

    const SenderConfig& config_;

    const rtp::FormatMap& format_map_;

    packet::PacketFactory& packet_factory_;
    core::BufferFactory<uint8_t>& byte_buffer_factory_;
    core::BufferFactory<audio::sample_t>& sample_buffer_factory_;

    core::IAllocator& allocator_;

    core::Optional<SenderEndpoint> source_endpoint_;
    core::Optional<SenderEndpoint> repair_endpoint_;

    core::Optional<packet::Router> router_;

    core::Optional<packet::Interleaver> interleaver_;

    core::ScopedPtr<fec::IBlockEncoder> fec_encoder_;
    core::Optional<fec::Writer> fec_writer_;

    core::ScopedPtr<audio::IFrameEncoder> payload_encoder_;
    core::Optional<audio::Packetizer> packetizer_;

    core::Optional<audio::PoisonWriter> resampler_poisoner_;
    core::Optional<audio::ResamplerWriter> resampler_writer_;
    core::ScopedPtr<audio::IResampler> resampler_;

    audio::IWriter* audio_writer_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_ENDPOINT_SET_H_
