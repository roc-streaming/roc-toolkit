/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/sample_format.h
//! @brief Sample format.

#ifndef ROC_AUDIO_SAMPLE_FORMAT_H_
#define ROC_AUDIO_SAMPLE_FORMAT_H_

#include "roc_audio/pcm_format.h"

namespace roc {
namespace audio {

//! Sample format.
//! Defines representation of samples in memory.
//! Does not define sample rate and channel set.
enum SampleFormat {
    //! Invalid format.
    SampleFormat_Invalid,

    //! Interleaved PCM format.
    //! What specific PCM coding and endian is used is defined
    //! by PcmFormat enum.
    SampleFormat_Pcm,
};

//! Get string name of sample format.
const char* sample_format_to_str(SampleFormat format);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SAMPLE_FORMAT_H_
