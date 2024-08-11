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
#include "roc_dbgio/csv_dumper.h"
#include "roc_packet/ireader.h"

namespace roc {
namespace audio {

//! Metrics of depacketizer.
struct DepacketizerMetrics {
    //! Cumulative count of packets from which we decoded samples.
    uint64_t decoded_packets;

    //! Cumulative count of decoded samples.
    uint64_t decoded_samples;

    //! Cumulative count of samples missing due to losses or delays.
    uint64_t missing_samples;

    //! Cumulative count of packets dropped because they were late.
    //! @note
    //!  This metric includes packets that were only partially late.
    uint64_t late_packets;

    //! Cumulative count of samples in late packets.
    uint64_t late_samples;

    //! Cumulative count of packets repaired by FEC.
    //! @note
    //!  This metric excludes late packets that were repaired but then dropped.
    uint64_t recovered_packets;

    //! Cumulative count of samples in recovered packets.
    uint64_t recovered_samples;

    DepacketizerMetrics()
        : decoded_packets(0)
        , decoded_samples(0)
        , missing_samples(0)
        , late_packets(0)
        , late_samples(0)
        , recovered_packets(0)
        , recovered_samples(0) {
    }
};

//! Depacketizer.
//!
//! Reads packets from a packet reader, decodes samples from packets using a
//! frame decoder, and produces an audio stream of frames.
//!
//! Notes:
//!
//!  - Depacketizer assume that packets from packet reader come in correct
//!    order, i.e. next packet has higher timestamp that previous one.
//!
//!  - If this assumption breaks and a outdated packet is fetched from packet
//!    reader, it's dropped.
//!
//!  - Depacketizer uses ModePeek to see what is the next available packet in
//!    packet reader. It doesn't use ModeFetch until next packet is actually
//!    used, to give late packets more time to arrive.
//!
//!  - In ModeHard, depacketizer fills gaps caused by packet losses with zeros.
//!
//!  - In ModeSoft, depacketizer stops reading at the first gap and returns
//!    either StatusPart or StatusDrain.
//!
//!  - Depacketizer never mixes decoded samples and gaps in same frame. E.g. if
//!    100 samples are requested, and first 20 samples are missing, depacketizer
//!    generates two partial reads: first with 20 zeroized samples, second with
//!    80 decoded samples.
class Depacketizer : public IFrameReader, public core::NonCopyable<> {
public:
    //! Initialization.
    Depacketizer(packet::IReader& packet_reader,
                 IFrameDecoder& payload_decoder,
                 FrameFactory& frame_factory,
                 const SampleSpec& sample_spec,
                 dbgio::CsvDumper* dumper);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Did depacketizer catch first packet?
    bool is_started() const;

    //! Get metrics.
    const DepacketizerMetrics& metrics() const;

    //! Get next timestamp to be rendered.
    //! @pre
    //!  is_started() should return true
    packet::stream_timestamp_t next_timestamp() const;

    //! Read audio frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(Frame& frame, packet::stream_timestamp_t duration, FrameReadMode mode);

private:
    // Statistics collected during decoding of one frame.
    struct FrameStats {
        // Total number of samples written to frame.
        size_t n_written_samples;

        // How much of all samples written to frame were decoded from packets.
        size_t n_decoded_samples;

        // How much of all samples written to frame were missing and zeroized.
        size_t n_missing_samples;

        // Number of packets dropped during decoding of this frame.
        size_t n_dropped_packets;

        // This frame first sample timestamp.
        core::nanoseconds_t capture_ts;

        FrameStats()
            : n_written_samples(0)
            , n_decoded_samples(0)
            , n_missing_samples(0)
            , n_dropped_packets(0)
            , capture_ts(0) {
        }
    };

    sample_t* read_samples_(sample_t* buff_ptr,
                            sample_t* buff_end,
                            FrameReadMode mode,
                            FrameStats& stats);

    sample_t* read_decoded_samples_(sample_t* buff_ptr, sample_t* buff_end);
    sample_t* read_missing_samples_(sample_t* buff_ptr, sample_t* buff_end);

    status::StatusCode
    update_packet_(size_t requested_samples, FrameReadMode mode, FrameStats& stats);
    status::StatusCode fetch_packet_(size_t requested_samples, FrameReadMode mode);
    status::StatusCode start_packet_();

    void commit_frame_(Frame& frame, size_t frame_samples, const FrameStats& stats);

    void periodic_report_();
    void dump_();

    FrameFactory& frame_factory_;
    packet::IReader& packet_reader_;
    IFrameDecoder& payload_decoder_;

    const SampleSpec sample_spec_;

    packet::PacketPtr packet_;

    packet::stream_timestamp_t stream_ts_;
    core::nanoseconds_t next_capture_ts_;
    bool valid_capture_ts_;

    size_t decoded_samples_;
    size_t missing_samples_;
    size_t late_samples_;
    size_t recovered_samples_;

    DepacketizerMetrics metrics_;

    bool is_started_;

    core::RateLimiter rate_limiter_;
    dbgio::CsvDumper* dumper_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_DEPACKETIZER_H_
