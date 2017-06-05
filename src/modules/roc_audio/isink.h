/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/isink.h
//! @brief Sink interface.

#ifndef ROC_AUDIO_ISINK_H_
#define ROC_AUDIO_ISINK_H_

#include "roc_audio/istream_reader.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Sink interface.
class ISink {
public:
    virtual ~ISink();

    //! Attach reader for channel.
    virtual void attach(packet::channel_t, IStreamReader&) = 0;

    //! Detach reader for channel.
    virtual void detach(packet::channel_t, IStreamReader&) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_ISINK_H_
