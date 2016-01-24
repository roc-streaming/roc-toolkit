/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/streamer.h
//! @brief Streamer.

#ifndef ROC_AUDIO_STREAMER_H_
#define ROC_AUDIO_STREAMER_H_

#include "roc_core/noncopyable.h"
#include "roc_core/timer.h"

#include "roc_packet/ipacket_reader.h"
#include "roc_packet/ipacket.h"
#include "roc_packet/units.h"

#include "roc_audio/istream_reader.h"

namespace roc {
namespace audio {

//! Streamer.
//!
//! Reads audio packets from input queue for a channel and produces continous
//! stream of audio samples:
//!  - copies samples from audio packets to output stream using timestamp
//!    field as positional number of first sample in packet;
//!  - fills stream gaps (missing packets) with zeros;
//!  - drops late packets;
//!  - handles overlapping packets.
class Streamer : public IStreamReader, public core::NonCopyable<> {
public:
    //! Initializer.
    //!
    //! @b Parameters
    //!  - @p reader is input queue of audio packets;
    //!  - @p channel is channel number for which packets should be read;
    //!  - @p beep defines whether missing samples should be replaces with a beep.
    Streamer(packet::IPacketReader& reader, packet::channel_t channel, bool beep = false);

    //! Read samples.
    virtual void read(const ISampleBufferSlice&);

private:
    typedef packet::sample_t sample_t;

    void update_packet_();

    packet::IPacketConstPtr read_packet_();

    sample_t* read_samples_(sample_t* begin, sample_t* end);

    sample_t* read_packet_samples_(sample_t* begin, sample_t* end);
    sample_t* read_missing_samples_(sample_t* begin, sample_t* end);

    packet::IPacketReader& reader_;
    packet::channel_t channel_;

    packet::IPacketConstPtr packet_;
    packet::timestamp_t packet_pos_;

    packet::timestamp_t timestamp_;

    packet::timestamp_t zero_samples_;
    packet::timestamp_t missing_samples_;
    packet::timestamp_t packet_samples_;

    core::Timer timer_;

    bool first_packet_;
    bool beep_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_STREAMER_H_
