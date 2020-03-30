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

#include "roc_audio/iframe_encoder.h"
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
    //!
    //! @b Parameters
    //!  - @p writer is used to write generated packets
    //!  - @p composer is used to initialize new packets
    //!  - @p payload_encoder is used to write samples to packets
    //!  - @p packet_pool is used to allocate packets
    //!  - @p buffer_pool is used to allocate buffers for packets
    //!  - @p channels defines a set of channels in the input frames
    //!  - @p packet_length defines packet length in nanoseconds
    //!  - @p sample_rate defines number of samples per channel per second
    //!  - @p payload_type defines packet payload type
    Packetizer(packet::IWriter& writer,
               packet::IComposer& composer,
               IFrameEncoder& payload_encoder,
               packet::PacketPool& packet_pool,
               core::BufferPool<uint8_t>& buffer_pool,
               packet::channel_mask_t channels,
               core::nanoseconds_t packet_length,
               size_t sample_rate,
               unsigned int payload_type);

    //! Write audio frame.
    virtual void write(Frame& frame);

    //! Flush buffered packet, if any.
    //! @remarks
    //!  Packet is padded to match fixed size.
    void flush();

    //! Check if object is successfully constructed.
    bool valid() const;

private:
    bool begin_packet_();
    void end_packet_();

    void pad_packet_();

    packet::PacketPtr create_packet_();

    packet::IWriter& writer_;
    packet::IComposer& composer_;
    IFrameEncoder& payload_encoder_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& buffer_pool_;

    const packet::channel_mask_t channels_;
    const size_t num_channels_;
    const size_t samples_per_packet_;
    const unsigned int payload_type_;
    const size_t payload_size_;

    packet::PacketPtr packet_;
    size_t packet_pos_;

    packet::source_t source_;
    packet::seqnum_t seqnum_;
    packet::timestamp_t timestamp_;

    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PACKETIZER_H_
