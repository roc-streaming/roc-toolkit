/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/iframe_writer.h
//! @brief Frame writer interface.

#ifndef ROC_AUDIO_IFRAME_WRITER_H_
#define ROC_AUDIO_IFRAME_WRITER_H_

#include "roc_audio/frame.h"
#include "roc_core/attributes.h"
#include "roc_core/list_node.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

//! Frame writer interface.
class IFrameWriter : public core::ListNode<> {
public:
    virtual ~IFrameWriter();

    //! Write frame.
    //!
    //! @returns
    //!  - If frame was successfully and completely written, returns status::StatusOK,
    //!    otherwise, returns an error.
    //!  - In case of error, it's not guaranteed that pipeline state didn't change,
    //!    e.g. part of the frame may be written.
    //!
    //! @see status::StatusCode.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(Frame& frame) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IFRAME_WRITER_H_
