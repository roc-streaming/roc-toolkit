/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/iwriter.h
//! @brief Audio writer interface.

#ifndef ROC_AUDIO_IWRITER_H_
#define ROC_AUDIO_IWRITER_H_

#include "roc_audio/frame.h"
#include "roc_core/list_node.h"

namespace roc {
namespace audio {

//! Audio writer interface.
class IWriter : public core::ListNode {
public:
    virtual ~IWriter();

    //! Write audio frame.
    virtual void write(Frame& frame) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IWRITER_H_
