/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/splitter.h
//! @brief Splitter.

#ifndef ROC_AUDIO_SPLITTER_H_
#define ROC_AUDIO_SPLITTER_H_

#include "roc_config/config.h"

#include "roc_core/noncopyable.h"

#include "roc_packet/units.h"
#include "roc_packet/ipacket_writer.h"
#include "roc_packet/ipacket_composer.h"
#include "roc_packet/ipacket.h"

#include "roc_audio/isample_buffer_writer.h"

namespace roc {
namespace audio {

//! Splitter.
//! @remarks
//!  Splits audio stream into sequence of audio packets and writes
//!  them to output packet writer.
class Splitter : public ISampleBufferWriter, public core::NonCopyable<> {
public:
    //! Initializer.
    //!
    //! @b Parameters
    //!  - @p output is used to write constructed packets;
    //!  - @p composer is used to construct audio packets;
    //!  - @p samples specifies number of samples per channel in packet;
    //!  - @p channels specifies bitmask of enabled audio channels.
    Splitter(packet::IPacketWriter& output,
             packet::IPacketComposer& composer,
             size_t samples = ROC_CONFIG_DEFAULT_PACKET_SAMPLES,
             packet::channel_mask_t channels = ROC_CONFIG_DEFAULT_CHANNEL_MASK,
             size_t rate = ROC_CONFIG_DEFAULT_SAMPLE_RATE);

    //! Write samples.
    //! @remarks
    //!  @p buffer should contain any number of samples in interleaved
    //!  format for all enabled channels.
    virtual void write(const ISampleBufferConstSlice& buffer);

    //! Flush buffered packet.
    //! @remarks
    //!  Packet is padded with zero samples to match fixed size.
    void flush();

private:
    bool create_packet_();

    packet::IPacketWriter& output_;
    packet::IPacketComposer& composer_;

    const packet::channel_mask_t channels_;
    const size_t n_channels_;
    const size_t n_packet_samples_;
    const size_t rate_;

    packet::IPacketPtr packet_;
    const packet::source_t source_;
    packet::seqnum_t seqnum_;
    packet::timestamp_t timestamp_;

    size_t n_samples_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SPLITTER_H_
