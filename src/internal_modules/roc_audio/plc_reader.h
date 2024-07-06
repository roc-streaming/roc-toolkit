/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/plc_reader.h
//! @brief PLC reader.

#ifndef ROC_AUDIO_PLC_READER_H_
#define ROC_AUDIO_PLC_READER_H_

#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/iplc.h"
#include "roc_audio/plc_config.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! Packet loss concealment (PLC) reader.
//!
//! Reads and forwards frames from underlying reader:
//!  - if returned frame has HasSignal flag (i.e. it's a good frame with samples
//!    decoded from packets), forwards frame as-is
//!  - if returned frame has HasGaps flag (i.e. it's a silence frame caused by
//!    a packet loss), asks IPlc to fill frame with interpolated data
//!
//! When a signal frame is retrieved, PLC reader passes it to IPlc, so that it can
//! remember it for later use.
//!
//! When a gap frame is retrieved, PLC reader performs read-ahead using a soft read
//! (ModeSoft). A soft read returns samples only if next packets already arrived.
//! This allows PLC reader to provide IPlc with the next frame when possible, without
//! increasing requirements for latency. IPlc may use the next frame to achieve
//! better results when doing interpolation.
//!
//! PLC reader expects that depacketizer never mixes signal and gaps and returns
//! frame that is entirely signal or gap. To achieve this, depacketizer uses
//! partial reads mechanism.
//!
//! PLC reader can work with arbitrary PCM format, specified by SampleSpec.
class PlcReader : public IFrameReader, public core::NonCopyable<> {
public:
    //! Initialize.
    PlcReader(IFrameReader& frame_reader,
              FrameFactory& frame_factory,
              IPlc& plc,
              const SampleSpec& sample_spec);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Read audio frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(Frame& frame, packet::stream_timestamp_t duration, FrameReadMode mode);

private:
    status::StatusCode read_from_memory_(Frame& frame,
                                         packet::stream_timestamp_t duration);
    status::StatusCode read_from_reader_(Frame& frame,
                                         packet::stream_timestamp_t duration,
                                         FrameReadMode mode);

    status::StatusCode try_read_next_frame_();
    status::StatusCode build_prev_frame_();

    void append_history_(Frame& frame);

    FrameFactory& frame_factory_;
    IFrameReader& frame_reader_;

    IPlc& plc_;

    // IPlc window lengths.
    const packet::stream_timestamp_t lookbehind_duration_;
    const size_t lookbehind_byte_size_;
    const packet::stream_timestamp_t lookahead_duration_;
    const size_t lookahead_byte_size_;

    // Holds history remembered from last read.
    FramePtr prev_frame_;
    FramePtr ring_frame_;
    size_t ring_frame_pos_;
    size_t ring_frame_size_;

    // Holds unread frame remembered from last read-ahead.
    // Subsequent reads will return samples from it until it's empty,
    // then switch to normal reads.
    bool pending_next_frame_;
    size_t next_frame_pos_;
    FramePtr next_frame_;
    FramePtr temp_frame_;

    // Set to true when we got the very first frame with signal.
    bool got_first_signal_;

    const SampleSpec sample_spec_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PLC_READER_H_
