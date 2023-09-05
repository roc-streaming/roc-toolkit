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
    //! Construct frame from samples.
    //! @remarks
    //!  The pointer is saved in the frame, no copying is performed.
    Frame(sample_t* samples, size_t num_samples);

    //! Frame flags.
    enum {
        //! Set if the frame has at least some samples from packets.
        //! If this flag is clear, frame is completely zero because of lack of packets.
        FlagNonblank = (1 << 0),

        //! Set if the frame is not fully filled with samples from packets.
        //! If this flag is set, frame is partially zero because of lack of packets.
        FlagIncomplete = (1 << 1),

        //! Set if some late packets were dropped while the frame was being built.
        //! It's not necessarty that the frame itself is blank or incomplete.
        FlagDrops = (1 << 2)
    };

    //! Set flags.
    void set_flags(unsigned flags);

    //! Get flags.
    unsigned flags() const;

    //! Get frame data.
    sample_t* samples() const;

    //! Get frame data size.
    size_t num_samples() const;

    //! Get unix-epoch timestamp in ns of the 1st sample.
    core::nanoseconds_t capture_timestamp() const;

    //! Set unix-epoch timestamp in ns of the 1st sample.
    void set_capture_timestamp(core::nanoseconds_t capture_ts);

    //! Print frame to stderr.
    void print() const;

private:
    sample_t* samples_;
    size_t num_samples_;
    unsigned flags_;
    core::nanoseconds_t capture_timestamp_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FRAME_H_
