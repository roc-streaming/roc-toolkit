/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/isample_buffer_writer.h
//! @brief Sample buffer writer interface.

#ifndef ROC_AUDIO_ISAMPLE_BUFFER_WRITER_H_
#define ROC_AUDIO_ISAMPLE_BUFFER_WRITER_H_

#include "roc_audio/sample_buffer.h"

namespace roc {
namespace audio {

//! Stream writer interface.
class ISampleBufferWriter {
public:
    virtual ~ISampleBufferWriter();

    //! Write samples.
    virtual void write(const ISampleBufferConstSlice& buffer) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_ISAMPLE_BUFFER_WRITER_H_
