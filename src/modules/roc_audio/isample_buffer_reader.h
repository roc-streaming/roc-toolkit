/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/isample_buffer_reader.h
//! @brief Sample buffer reader interface.

#ifndef ROC_AUDIO_ISAMPLE_BUFFER_READER_H_
#define ROC_AUDIO_ISAMPLE_BUFFER_READER_H_

#include "roc_audio/sample_buffer.h"

namespace roc {
namespace audio {

//! Sample reader interface.
class ISampleBufferReader {
public:
    virtual ~ISampleBufferReader();

    //! Read buffer.
    virtual ISampleBufferConstSlice read() = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_ISAMPLE_BUFFER_READER_H_
