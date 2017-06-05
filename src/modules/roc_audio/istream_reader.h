/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/istream_reader.h
//! @brief Stream reader interface.

#ifndef ROC_AUDIO_ISTREAM_READER_H_
#define ROC_AUDIO_ISTREAM_READER_H_

#include "roc_audio/sample_buffer.h"
#include "roc_core/list_node.h"

namespace roc {
namespace audio {

//! Stream reader interface.
class IStreamReader : public core::ListNode {
public:
    virtual ~IStreamReader();

    //! Read samples.
    //! @remarks
    //!  Fills @p buffer with next buffer->size() samples.
    virtual void read(const ISampleBufferSlice& buffer) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_ISTREAM_READER_H_
