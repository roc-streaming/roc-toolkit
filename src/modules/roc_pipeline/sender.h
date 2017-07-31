/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/sender.h
//! @brief Sender pipeline.

#ifndef ROC_PIPELINE_SENDER_H_
#define ROC_PIPELINE_SENDER_H_

#include "roc_audio/iencoder.h"
#include "roc_audio/iwriter.h"
#include "roc_audio/packetizer.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ticker.h"
#include "roc_core/unique_ptr.h"
#include "roc_fec/iencoder.h"
#include "roc_fec/writer.h"
#include "roc_packet/interleaver.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/router.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/sender_port.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {

//! Sender pipeline.
class Sender : public audio::IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    Sender(const SenderConfig& config,
           packet::IWriter& source_writer,
           packet::IWriter& repair_writer,
           const rtp::FormatMap& format_map,
           packet::PacketPool& packet_pool,
           core::BufferPool<uint8_t>& buffer_pool,
           core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid();

    //! Write audio frame.
    virtual void write(audio::Frame& frame);

private:
    core::UniquePtr<SenderPort> source_port_;
    core::UniquePtr<SenderPort> repair_port_;

    core::UniquePtr<packet::Router> router_;

    core::UniquePtr<packet::Interleaver> interleaver_;

    core::UniquePtr<fec::IEncoder> fec_encoder_;
    core::UniquePtr<fec::Writer> fec_writer_;

    core::UniquePtr<audio::IEncoder> encoder_;
    core::UniquePtr<audio::Packetizer> packetizer_;

    core::Ticker ticker_;
    bool timing_;

    packet::timestamp_t timestamp_;
    size_t num_channels_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_H_
