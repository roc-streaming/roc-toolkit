/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
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
#include "roc_core/slice.h"

namespace roc {
namespace audio {

//! Audio frame.
class Frame {
public:
    //! Construct empty frame.
    Frame();

    //! Construct frame from samples.
    Frame(const core::Slice<sample_t>& samples);

    //! Frame flags.
    enum {
        //! Set if no packets were extracted to the frame when the frame was built.
        FlagEmpty = (1 << 0),

        //! Set if some queued packets were regarded as outdated and dropped when the
        //! frame was built.
        FlagSkip = (1 << 1)
    };

    //! Add flags.
    void add_flags(unsigned flags);

    //! Returns true if frame has no packets.
    bool is_empty() const;

    //! Returns true if some packets were dropped when frame was built.
    bool has_skip() const;

    //! Get frame samples.
    const core::Slice<sample_t>& samples() const;

    //! Set frame samples.
    void set_samples(const core::Slice<sample_t>& samples);

private:
    core::Slice<sample_t> samples_;

    unsigned flags_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FRAME_H_
