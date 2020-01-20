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

#include "roc_audio/iframe_encoder.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/packetizer.h"
#include "roc_audio/poison_writer.h"
#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_writer.h"
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
#include "roc_pipeline/sender_port.h"
#include "roc_rtp/format_map.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {

//! Sender sink pipeline.
//! @remarks
//   - input: frames
//!  - output: packets
class SenderSink : public sndio::ISink, public core::NonCopyable<> {
public:
    //! Initialize.
    SenderSink(const SenderConfig& config,
               const PortConfig& source_port,
               packet::IWriter& source_writer,
               const PortConfig& repair_port,
               packet::IWriter& repair_writer,
               const fec::CodecMap& codec_map,
               const rtp::FormatMap& format_map,
               packet::PacketPool& packet_pool,
               core::BufferPool<uint8_t>& byte_buffer_pool,
               core::BufferPool<audio::sample_t>& sample_buffer_pool,
               core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid();

    //! Get sink sample rate.
    virtual size_t sample_rate() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Write audio frame.
    virtual void write(audio::Frame& frame);

private:
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

    core::ScopedPtr<audio::PoisonWriter> pipeline_poisoner_;

    core::ScopedPtr<core::Ticker> ticker_;

    audio::IWriter* audio_writer_;

    SenderConfig config_;

    packet::timestamp_t timestamp_;
    size_t num_channels_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_SINK_H_
