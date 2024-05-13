/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/iframe_reader.h
//! @brief Frame reader interface.

#ifndef ROC_AUDIO_IFRAME_READER_H_
#define ROC_AUDIO_IFRAME_READER_H_

#include "roc_audio/frame.h"
#include "roc_core/list_node.h"

namespace roc {
namespace audio {

//! Frame reader interface.
class IFrameReader : public core::ListNode<> {
public:
    virtual ~IFrameReader();

    //! Read audio frame.
    //! @remarks
    //!  Frame buffer and its size should be set by caller. The reader
    //!  should fill the entire buffer and should not resize it.
    //! @returns
    //!  false if there is nothing to read anymore.
    virtual bool read(Frame& frame) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IFRAME_READER_H_
