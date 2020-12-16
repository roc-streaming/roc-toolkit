/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/depacketizer.h
//! @brief Depacketizer.

#ifndef ROC_AUDIO_DEPACKETIZER_H_
#define ROC_AUDIO_DEPACKETIZER_H_

#include "roc_audio/iframe_decoder.h"
#include "roc_audio/ireader.h"
#include "roc_audio/units.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_packet/ireader.h"

namespace roc {
namespace audio {

//! Depacketizer.
//! @remarks
//!  Reads packets from a packet reader, decodes samples from packets using a
//!  decoder, and produces an audio stream.
class Depacketizer : public IReader, public core::NonCopyable<> {
public:
    //! Initialization.
    //!
    //! @b Parameters
    //!  - @p reader is used to read packets
    //!  - @p payload_decoder is used to extract samples from packets
    //!  - @p channels defines a set of channels in the output frames
    //!  - @p beep enables weird beeps instead of silence on packet loss
    Depacketizer(packet::IReader& reader,
                 IFrameDecoder& payload_decoder,
                 packet::channel_mask_t channels,
                 bool beep);

    //! Read audio frame.
    virtual ssize_t read(Frame& frame);

    //! Did depacketizer catch first packet?
    bool started() const;

    //! Get next timestamp to be rendered.
    //! @pre
    //!  started() should return true
    packet::timestamp_t timestamp() const;

private:
    void read_frame_(Frame& frame);

    sample_t* read_samples_(sample_t* buff_ptr, sample_t* buff_end);

    sample_t* read_packet_samples_(sample_t* buff_ptr, sample_t* buff_end);
    sample_t* read_missing_samples_(sample_t* buff_ptr, sample_t* buff_end);

    void set_frame_flags_(Frame& frame,
                          size_t prev_dropped_packets,
                          packet::timestamp_t prev_packet_samples);

    void update_packet_();
    packet::PacketPtr read_packet_();

    packet::IReader& reader_;
    IFrameDecoder& payload_decoder_;

    const packet::channel_mask_t channels_;
    const size_t num_channels_;

    packet::PacketPtr packet_;

    packet::timestamp_t timestamp_;

    packet::timestamp_t zero_samples_;
    packet::timestamp_t missing_samples_;
    packet::timestamp_t packet_samples_;

    core::RateLimiter rate_limiter_;

    bool first_packet_;
    bool beep_;

    size_t dropped_packets_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_DEPACKETIZER_H_
