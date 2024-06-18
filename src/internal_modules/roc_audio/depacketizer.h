/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/depacketizer.h
//! @brief Depacketizer.

#ifndef ROC_AUDIO_DEPACKETIZER_H_
#define ROC_AUDIO_DEPACKETIZER_H_

#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_decoder.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_packet/ireader.h"

namespace roc {
namespace audio {

//! Depacketizer.
//! @remarks
//!  Reads packets from a packet reader, decodes samples from packets using a
//!  decoder, and produces an audio stream.
class Depacketizer : public IFrameReader, public core::NonCopyable<> {
public:
    //! Initialization.
    //!
    //! @b Parameters
    //!  - @p packet_reader is used to read packets
    //!  - @p payload_decoder is used to extract samples from packets
    //!  - @p sample_spec describes output frames
    //!  - @p beep enables weird beeps instead of silence on packet loss
    Depacketizer(packet::IReader& packet_reader,
                 IFrameDecoder& payload_decoder,
                 FrameFactory& frame_factory,
                 const SampleSpec& sample_spec,
                 bool beep);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Did depacketizer catch first packet?
    bool is_started() const;

    //! Read audio frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(Frame& frame, packet::stream_timestamp_t duration, FrameReadMode mode);

    //! Get next timestamp to be rendered.
    //! @pre
    //!  is_started() should return true
    packet::stream_timestamp_t next_timestamp() const;

private:
    struct FrameStats {
        // Number of samples decoded from packets into the frame.
        size_t n_decoded_samples;

        // Number of samples filled out in the frame.
        size_t n_filled_samples;

        // Number of packets dropped during frame construction.
        size_t n_dropped_packets;

        // This frame first sample timestamp.
        core::nanoseconds_t capture_ts;

        FrameStats()
            : n_decoded_samples(0)
            , n_filled_samples(0)
            , n_dropped_packets(0)
            , capture_ts(0) {
        }
    };

    sample_t* read_samples_(sample_t* buff_ptr, sample_t* buff_end, FrameStats& stats);

    sample_t* read_packet_samples_(sample_t* buff_ptr, sample_t* buff_end);
    sample_t* read_missing_samples_(sample_t* buff_ptr, sample_t* buff_end);

    status::StatusCode update_packet_(FrameStats& frame_stats);
    status::StatusCode fetch_packet_();
    bool start_packet_();

    void commit_frame_(Frame& frame, const FrameStats& stats);

    void report_stats_();

    FrameFactory& frame_factory_;
    packet::IReader& packet_reader_;
    IFrameDecoder& payload_decoder_;

    const SampleSpec sample_spec_;

    packet::PacketPtr packet_;

    packet::stream_timestamp_t stream_ts_;
    core::nanoseconds_t next_capture_ts_;
    bool valid_capture_ts_;

    size_t padding_samples_;
    size_t missing_samples_;
    size_t decoded_samples_;

    size_t fetched_packets_;
    size_t dropped_packets_;

    core::RateLimiter rate_limiter_;

    const bool beep_;

    bool first_packet_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_DEPACKETIZER_H_
