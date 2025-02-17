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
    //! Incremented each time packetizer starts encoding a packet.
    uint64_t encoded_packets;

    //! Cumulative count of encoded payload bytes.
    //! This excludes packet headers and padding.
    uint64_t payload_bytes;

    PacketizerMetrics()
        : encoded_packets(0)
        , payload_bytes(0) {
    }
};

//! Packetizer.
//! @remarks
//!  Gets an audio stream, encodes samples to packets using an encoder, and
//!  writes packets to a packet writer.
class Packetizer : public IFrameWriter, public core::NonCopyable<> {
public:
    //! Initialization.
    Packetizer(packet::IWriter& writer,
               packet::IComposer& composer,
               packet::ISequencer& sequencer,
               IFrameEncoder& payload_encoder,
               packet::PacketFactory& packet_factory,
               core::nanoseconds_t packet_length,
               const SampleSpec& sample_spec);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Get sample rate.
    size_t sample_rate() const;

    //! Get metrics.
    const PacketizerMetrics& metrics() const;

    //! Write audio frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(Frame& frame);

    //! Flush buffered packet, if any.
    //! @remarks
    //!  Packet is padded to match fixed size.
    ROC_ATTR_NODISCARD status::StatusCode flush();

private:
    status::StatusCode begin_packet_();
    status::StatusCode end_packet_();

    status::StatusCode create_packet_();
    void pad_packet_(size_t written_payload_size);

    packet::IWriter& writer_;
    packet::IComposer& composer_;
    packet::ISequencer& sequencer_;
    IFrameEncoder& payload_encoder_;

    packet::PacketFactory& packet_factory_;

    const SampleSpec sample_spec_;
    size_t samples_per_packet_;
    size_t payload_size_;

    packet::PacketPtr packet_;
    size_t packet_pos_;
    core::nanoseconds_t packet_cts_;

    core::nanoseconds_t capture_ts_;

    PacketizerMetrics metrics_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PACKETIZER_H_
