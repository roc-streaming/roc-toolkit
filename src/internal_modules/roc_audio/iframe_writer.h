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
#include "roc_core/list_node.h"

namespace roc {
namespace audio {

//! Frame writer interface.
class IFrameWriter : public core::ListNode {
public:
    virtual ~IFrameWriter();

    //! Write audio frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(Frame& frame) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IFRAME_WRITER_H_
