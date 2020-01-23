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
#include "roc_fec/codec_map.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_fec/writer.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/router.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/sender_port_group.h"
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
               const fec::CodecMap& codec_map,
               const rtp::FormatMap& format_map,
               packet::PacketPool& packet_pool,
               core::BufferPool<uint8_t>& byte_buffer_pool,
               core::BufferPool<audio::sample_t>& sample_buffer_pool,
               core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid() const;

    //! Port group identifier.
    typedef uintptr_t PortGroupID;

    //! Port identifier.
    typedef uintptr_t PortID;

    //! Add new port group.
    //! @returns
    //!  non-zero identifier on success or zero on error.
    PortGroupID add_port_group();

    //! Add port to group.
    PortID add_port(PortGroupID port_group,
                    address::EndpointType port_type,
                    const pipeline::PortConfig& port_config);

    //! Attach port to packet writer.
    void set_port_writer(PortID port_id, packet::IWriter& port_writer);

    //! Check if port group is fully configured.
    //! This is true when all necessary ports are added and attached to writers.
    bool port_group_configured(PortGroupID port_group) const;

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

    const fec::CodecMap& codec_map_;
    const rtp::FormatMap& format_map_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& byte_buffer_pool_;
    core::BufferPool<audio::sample_t>& sample_buffer_pool_;

    core::IAllocator& allocator_;

    core::List<SenderPortGroup> port_groups_;

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
