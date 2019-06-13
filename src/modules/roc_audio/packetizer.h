/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/packetizer.h
//! @brief Packetizer.

#ifndef ROC_AUDIO_PACKETIZER_H_
#define ROC_AUDIO_PACKETIZER_H_

#include "roc_audio/iencoder.h"
#include "roc_audio/iwriter.h"
#include "roc_audio/units.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Packetizer.
//! @remarks
//!  Gets an audio stream, encodes samples to packets using an encoder, and
//!  writes packets to a packet writer.
class Packetizer : public IWriter, public core::NonCopyable<> {
public:
    //! Initialization.
    Packetizer(packet::IWriter& writer,
               packet::IComposer& composer,
               IEncoder& encoder,
               packet::PacketPool& packet_pool,
               core::BufferPool<uint8_t>& buffer_pool,
               packet::channel_mask_t channels,
               core::nanoseconds_t packet_length,
               size_t sample_rate);

    //! Write audio frame.
    virtual void write(Frame& frame);

    //! Flush buffered packet, if any.
    //! @remarks
    //!  Packet payload is padded if necessary to match configured payload size.
    void flush();

private:
    bool begin_packet_();
    void end_packet_();

    packet::PacketPtr create_packet_();

    packet::IWriter& writer_;
    packet::IComposer& composer_;
    IEncoder& encoder_;
    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& buffer_pool_;

    const packet::channel_mask_t channels_;
    const size_t num_channels_;
    const size_t samples_per_packet_;

    packet::PacketPtr packet_;
    size_t remaining_samples_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PACKETIZER_H_
