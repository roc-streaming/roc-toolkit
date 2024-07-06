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
#include "roc_core/ipool.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/slice.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

class Frame;

//! Frame smart pointer.
typedef core::SharedPtr<Frame> FramePtr;

//! Frame.
class Frame : public core::RefCounted<Frame, core::PoolAllocation> {
public:
    //! Frame flags.
    //! Flags are designed the way so that if you combine multiple frames into one,
    //! (concatenate or mix), bitwise OR of their flags will give flags for resulting
    //! frame. E.g., if at least one frame has holes, combined frame has holes as
    //! well, if at least one frame has signal, combined frame also has signal, etc.
    enum Flags {
        //! Set if the frame has at least some samples filled from packets.
        //! If this flag is clear, frame is completely zero because of lack of packets.
        HasSignal = (1 << 0),

        //! Set if the frame is not fully filled with samples from packets.
        //! If this flag is set, frame is partially zero because of lack of packets.
        HasGaps = (1 << 1),

        //! Set if some late packets were dropped while the frame was being built.
        //! It's not necessarily that the frame itself has no signal or has holes.
        HasDrops = (1 << 2)
    };

    //! Construct empty frame.
    //! @remarks
    //!  Initially frame does not have a buffer and flags are zero.
    Frame(core::IPool& frame_pool);

    //! Clear all state.
    void clear();

    //! Get flags.
    unsigned flags() const;

    //! Check if frame has all of the given flags.
    bool has_flags(unsigned flags) const;

    //! Set flags.
    void set_flags(unsigned flags);

    //! Get underlying buffer.
    //! Returned buffer is used by raw_samples() and bytes().
    const core::Slice<uint8_t>& buffer();

    //! Attach underlying buffer.
    //! Attached buffer is used by raw_samples() and bytes().
    void set_buffer(const core::Slice<uint8_t>& buffer);

    //! Check frame is in raw format.
    bool is_raw() const;

    //! Mark on unmark frame to be in raw format.
    void set_raw(bool raw);

    //! Get frame data as raw samples.
    //! May be used only if is_raw() is true, otherwise use bytes().
    sample_t* raw_samples() const;

    //! Get number of raw samples in frame.
    //! May be used only if is_raw() is true, otherwise use num_bytes().
    size_t num_raw_samples() const;

    //! Set number of raw samples in frame.
    //! Resizes underlying buffer attached to frame.
    //! May be used only if is_raw() is true and buffer() is set.
    //! @p n_samples must be within buffer capacity.
    void set_num_raw_samples(size_t n_samples);

    //! Get frame data as bytes.
    uint8_t* bytes() const;

    //! Get number of bytes in frame.
    size_t num_bytes() const;

    //! Set number of bytes in frame.
    //! Resizes underlying buffer attached to frame.
    //! May be used only if buffer() is set.
    //! @p n_bytes must be within buffer capacity.
    void set_num_bytes(size_t n_bytes);

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
    core::Slice<uint8_t> buffer_;
    uint16_t is_raw_;
    uint16_t flags_;
    packet::stream_timestamp_t duration_;
    core::nanoseconds_t capture_timestamp_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FRAME_H_
