/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/ituner.h
//! @brief Tuner interface.

#ifndef ROC_AUDIO_ITUNER_H_
#define ROC_AUDIO_ITUNER_H_

#include "roc_core/list_node.h"
#include "roc_packet/iaudio_packet.h"

namespace roc {
namespace audio {

//! Tuner interface.
class ITuner : public core::ListNode {
public:
    virtual ~ITuner();

    //! Update stream.
    //! @returns false if stream is broken and should be destroyed.
    virtual bool update() = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_ITUNER_H_
