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
struct Frame {
    //! Frame samples.
    core::Slice<sample_t> samples;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FRAME_H_
