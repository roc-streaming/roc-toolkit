/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/irenderer.h
//! @brief Renderer interface.

#ifndef ROC_AUDIO_IRENDERER_H_
#define ROC_AUDIO_IRENDERER_H_

#include "roc_audio/isink.h"

namespace roc {
namespace audio {

//! Renderer interface.
class IRenderer {
public:
    virtual ~IRenderer();

    //! Update renderer state.
    virtual bool update() = 0;

    //! Attach all readers to sink.
    virtual void attach(ISink& sink) = 0;

    //! Detach all readers from sink.
    virtual void detach(ISink& sink) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IRENDERER_H_
