/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/sender_sink.h
//! @brief Sender sink pipeline.

#ifndef ROC_PIPELINE_SENDER_SINK_H_
#define ROC_PIPELINE_SENDER_SINK_H_

#include "roc_audio/fanout.h"
#include "roc_audio/iframe_encoder.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/packetizer.h"
#include "roc_audio/poison_writer.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/ticker.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_fec/writer.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/router.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/sender_endpoint_set.h"
#include "roc_rtp/format_map.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {

//! Sender sink pipeline.
//! Thread-safe.
//! @remarks
//!  - input: frames
//!  - output: packets
class SenderSink : public sndio::ISink, public core::NonCopyable<> {
public:
    //! Initialize.
    SenderSink(const SenderConfig& config,
               const rtp::FormatMap& format_map,
               packet::PacketPool& packet_pool,
               core::BufferPool<uint8_t>& byte_buffer_pool,
               core::BufferPool<audio::sample_t>& sample_buffer_pool,
               core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid() const;

    //! Opaque endpoint set handle.
    typedef struct EndpointSetHandle* EndpointSetHandle;

    //! Opaque endpoint handle.
    typedef struct EndpointHandle* EndpointHandle;

    //! Add new endpoint set.
    //! @returns
    //!  NULL on on error or non-NULL opaque handle on success.
    EndpointSetHandle add_endpoint_set();

    //! Add endpoint to endpoint set.
    //! Each endpoint set can have one source and zero or one repair endpoint.
    //! The protocols of endpoints in one set should be compatible.
    //! @returns
    //!  NULL on on error or non-NULL opaque handle on success.
    EndpointHandle add_endpoint(EndpointSetHandle endpoint_set,
                                address::EndpointType type,
                                address::EndpointProtocol proto);

    //! Set writer to which endpoint will write packets.
    void set_endpoint_output_writer(EndpointHandle endpoint, packet::IWriter& writer);

    //! Set UDP address for output packets.
    void set_endpoint_destination_udp_address(EndpointHandle endpoint,
                                              const address::SocketAddr& addr);

    //! Check if endpoint set configuration is done.
    //! This is true when all necessary endpoints are added and configured.
    bool endpoint_set_ready(EndpointSetHandle endpoint_set) const;

    //! Get sink sample rate.
    virtual size_t sample_rate() const;

    //! Get number of channels for the sink.
    virtual size_t num_channels() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Write audio frame.
    virtual void write(audio::Frame& frame);

private:
    const SenderConfig config_;

    const rtp::FormatMap& format_map_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& byte_buffer_pool_;
    core::BufferPool<audio::sample_t>& sample_buffer_pool_;

    core::IAllocator& allocator_;

    core::List<SenderEndpointSet> endpoint_sets_;

    audio::Fanout fanout_;

    core::ScopedPtr<audio::PoisonWriter> pipeline_poisoner_;

    core::ScopedPtr<core::Ticker> ticker_;

    audio::IWriter* audio_writer_;

    packet::timestamp_t timestamp_;
    size_t num_channels_;

    core::Mutex mutex_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_SINK_H_
