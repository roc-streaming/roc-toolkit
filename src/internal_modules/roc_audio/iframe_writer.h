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
#include "roc_packet/units.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

//! Frame writer interface.
class IFrameWriter : public core::ListNode<> {
public:
    virtual ~IFrameWriter();

    //! Write frame.
    //!
    //! @note
    //!  - Write is NOT allowed to modify the frame or its buffer, only read it.
    //!  - Writer is NOT allowed to store a reference to the frame or its buffer
    //!    for later use. After writer returns, caller may modify or destroy
    //!    frame or buffer.
    //!
    //! @returns
    //!  - If frame was successfully and completely written, returns status::StatusOK.
    //!  - Otherwise, returns an error. In this case it's not known whether anything
    //!    were written or not, and no further writes are expected.
    //!
    //! @see status::StatusCode.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(Frame& frame) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IFRAME_WRITER_H_
