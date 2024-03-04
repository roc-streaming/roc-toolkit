/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/frame.h
//! @brief Audio frame.

#ifndef ROC_AUDIO_FRAME_H_
#define ROC_AUDIO_FRAME_H_

#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Audio frame.
class Frame : public core::NonCopyable<> {
public:
    //! Construct frame from raw samples.
    //! Flags are set to zero.
    Frame(sample_t* samples, size_t num_samples);

    //! Construct frame from bytes.
    //! Flags are set to zero.
    Frame(uint8_t* bytes, size_t num_bytes);

    //! Frame flags.
    //! Flags are designed the way so that if you combine multiple frames into one,
    //! (concatenate or mix), bitwise OR of their flags will give flags for resulting
    //! frame. E.g., if at least one frame was non-blank, combined frame will be
    //! non-blank, if at least one frame was incomplete, combined frame will be
    //! incomplete, etc.
    enum {
        //! Set if the frame has format different from raw samples.
        //! If this flag is set, only bytes() can be used, and raw_samples() panics.
        FlagNotRaw = (1 << 0),

        //! Set if the frame has at least some samples from packets.
        //! If this flag is clear, frame is completely zero because of lack of packets.
        FlagNotBlank = (1 << 1),

        //! Set if the frame is not fully filled with samples from packets.
        //! If this flag is set, frame is partially zero because of lack of packets.
        FlagNotComplete = (1 << 2),

        //! Set if some late packets were dropped while the frame was being built.
        //! It's not necessarily that the frame itself is blank or incomplete.
        FlagPacketDrops = (1 << 3)
    };

    //! Get flags.
    unsigned flags() const;

    //! Set flags.
    void set_flags(unsigned flags);

    //! Check frame data is raw samples.
    //! Returns true if FlagAltFormat is not set.
    bool is_raw() const;

    //! Get frame data as raw samples.
    //! May be used only if is_raw() is true, otherwise use bytes().
    sample_t* raw_samples() const;

    //! Get number of raw samples in frame,
    //! May be used only if is_raw() is true, otherwise use num_bytes().
    size_t num_raw_samples() const;

    //! Get frame data as bytes.
    uint8_t* bytes() const;

    //! Get number of bytes in frame.
    size_t num_bytes() const;

    //! Check if duration was set.
    bool has_duration() const;

    //! Get frame duration in terms of stream timestamps.
    packet::stream_timestamp_t duration() const;

    //! Get frame duration in terms of stream timestamps.
    void set_duration(packet::stream_timestamp_t duration);

    //! Check if capture timestamp is set.
    bool has_capture_timestamp() const;

    //! Get unix-epoch timestamp in ns of the 1st sample.
    core::nanoseconds_t capture_timestamp() const;

    //! Set unix-epoch timestamp in ns of the 1st sample.
    void set_capture_timestamp(core::nanoseconds_t capture_ts);

    //! Print frame to stderr.
    void print() const;

private:
    uint8_t* bytes_;
    size_t num_bytes_;
    unsigned flags_;
    packet::stream_timestamp_t duration_;
    core::nanoseconds_t capture_timestamp_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FRAME_H_
