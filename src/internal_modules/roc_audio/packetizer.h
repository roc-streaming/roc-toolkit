/*
 * Copyright (c) 2015 Roc Streaming authors
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
#include "roc_audio/iframe_writer.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/isequencer.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Metrics of packetizer.
struct PacketizerMetrics {
    //! Cumulative count of produced packets.
    uint64_t packet_count;

    //! Cumulative count of produced payload bytes.
    //! This excludes packet headers and padding.
    uint64_t payload_count;

    PacketizerMetrics()
        : packet_count(0)
        , payload_count(0) {
    }
};

//! Packetizer.
//! @remarks
//!  Gets an audio stream, encodes samples to packets using an encoder, and
//!  writes packets to a packet writer.
class Packetizer : public IFrameWriter, public core::NonCopyable<> {
public:
    //! Initialization.
    //!
    //! @b Parameters
    //!  - @p writer is used to write generated packets
    //!  - @p composer is used to initialize new packets
    //!  - @p sequencer is used to put packets in sequence
    //!  - @p payload_encoder is used to write samples to packets
    //!  - @p packet_factory is used to allocate packets
    //!  - @p buffer_factory is used to allocate buffers for packets
    //!  - @p packet_length defines packet length in nanoseconds
    //!  - @p sample_spec describes input frames
    Packetizer(packet::IWriter& writer,
               packet::IComposer& composer,
               packet::ISequencer& sequencer,
               IFrameEncoder& payload_encoder,
               packet::PacketFactory& packet_factory,
               core::BufferFactory<uint8_t>& buffer_factory,
               core::nanoseconds_t packet_length,
               const audio::SampleSpec& sample_spec);

    //! Check if object is successfully constructed.
    bool is_valid() const;

    //! Get sample rate.
    size_t sample_rate() const;

    //! Get metrics.
    PacketizerMetrics metrics() const;

    //! Write audio frame.
    virtual void write(Frame& frame);

    //! Flush buffered packet, if any.
    //! @remarks
    //!  Packet is padded to match fixed size.
    void flush();

private:
    bool begin_packet_();
    void end_packet_();

    void pad_packet_(size_t written_payload_size);

    packet::PacketPtr create_packet_();

    packet::IWriter& writer_;
    packet::IComposer& composer_;
    packet::ISequencer& sequencer_;
    IFrameEncoder& payload_encoder_;

    packet::PacketFactory& packet_factory_;
    core::BufferFactory<uint8_t>& buffer_factory_;

    const audio::SampleSpec sample_spec_;
    const size_t samples_per_packet_;
    const size_t payload_size_;

    packet::PacketPtr packet_;
    size_t packet_pos_;
    core::nanoseconds_t packet_cts_;

    core::nanoseconds_t capture_ts_;

    PacketizerMetrics metrics_;

    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PACKETIZER_H_
