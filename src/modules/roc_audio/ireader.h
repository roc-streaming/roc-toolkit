/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/ireader.h
//! @brief Audio reader interface.

#ifndef ROC_AUDIO_IREADER_H_
#define ROC_AUDIO_IREADER_H_

#include "roc_audio/frame.h"
#include "roc_core/list_node.h"

namespace roc {
namespace audio {

//! Audio reader interface.
class IReader : public core::ListNode {
public:
    virtual ~IReader();

    //! Read audio frame.
    //! @remarks
    //!  Frame buffer and its size should be set by caller. The reader
    //!  should fill the entire buffer and should not resize it.
    //! @returns
    //!  false zero or positive number of samples read or written, in range [0;
    //!  frame.size()) or negative error code defined in roc_error module
    //!  (roc_error/error_code.h).
    virtual ssize_t read(Frame& frame) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IREADER_H_
