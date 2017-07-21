/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
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
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace audio {

//! Packetizer.
//! @remarks
//!  Gets an audio stream, encodes samples to packets using an encoder, and
//!  writes packets to a packet writer.
class Packetizer : public IWriter, public core::NonCopyable<> {
public:
    //! Initialization.
    //!
    //! @b Parameters
    //!  - @p writer is used to write generated packets
    //!  - @p composer is used to initialize new packets
    //!  - @p encoder is used to write samples to packets
    //!  - @p packet_pool is used to allocate packets
    //!  - @p buffer_pool is used to allocate buffers for packets
    //!  - @p channels defines a set of channels in the input frames
    //!  - @p samples_per_packet defines number of samples per packet per channel
    //!  - @p payload_type defines packet payload type
    Packetizer(packet::IWriter& writer,
               packet::IComposer& composer,
               IEncoder& encoder,
               packet::PacketPool& packet_pool,
               core::BufferPool<uint8_t>& buffer_pool,
               packet::channel_mask_t channels,
               size_t samples_per_packet,
               unsigned int payload_type);

    //! Write audio frame.
    virtual void write(Frame& frame);

    //! Flush buffered packet, if any.
    //! @remarks
    //!  Packet is padded with zero samples to match fixed size.
    void flush();

private:
    packet::PacketPtr next_packet_();

    packet::IWriter& writer_;
    packet::IComposer& composer_;
    IEncoder& encoder_;
    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& buffer_pool_;

    const packet::channel_mask_t channels_;
    const size_t num_channels_;
    const size_t samples_per_packet_;
    const unsigned int payload_type_;

    packet::PacketPtr packet_;
    size_t packet_pos_;

    const packet::source_t source_;
    packet::seqnum_t seqnum_;
    packet::timestamp_t timestamp_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PACKETIZER_H_
