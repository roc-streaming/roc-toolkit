/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/frame.h
//! @brief Audio frame.

#ifndef ROC_AUDIO_FRAME_H_
#define ROC_AUDIO_FRAME_H_

#include "roc_audio/units.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! Audio frame.
class Frame : public core::NonCopyable<> {
public:
    //! Construct frame from samples.
    //! @remarks
    //!  The pointer is saved in the frame, no copying is performed.
    Frame(sample_t* data, size_t size);

    //! Frame flags.
    enum {
        //! Set if the frame is fully filled with zeros instead of data from packets.
        FlagBlank = (1 << 0),

        //! Set if the frame is partially filled with zeros instead of data from packets.
        FlagIncomplete = (1 << 1),

        //! Set if some late packets were dropped while the frame was being built.
        FlagDrops = (1 << 2)
    };

    //! Set flags.
    void set_flags(unsigned flags);

    //! Get flags.
    unsigned flags() const;

    //! Get frame data.
    sample_t* data() const;

    //! Get frame data size.
    size_t size() const;

    //! Print frame to stderr.
    void print() const;

private:
    sample_t* data_;
    size_t size_;
    unsigned flags_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FRAME_H_
