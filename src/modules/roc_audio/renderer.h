/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/renderer.h
//! @brief Renderer.

#ifndef ROC_AUDIO_RENDERER_H_
#define ROC_AUDIO_RENDERER_H_

#include "roc_config/config.h"

#include "roc_core/noncopyable.h"
#include "roc_core/list.h"
#include "roc_core/array.h"

#include "roc_audio/istream_reader.h"
#include "roc_audio/isink.h"
#include "roc_audio/ituner.h"
#include "roc_audio/irenderer.h"

namespace roc {
namespace audio {

//! Renderer.
class Renderer : public IRenderer, public core::NonCopyable<> {
public:
    Renderer();

    //! Set reader for given channel.
    //! @remarks
    //!  Only one reader per channel allowed. Reader will be attached
    //!  to sink in attach().
    void set_reader(packet::channel_t ch, IStreamReader& reader);

    //! Add tuner.
    //! @remarks
    //!  Tuner will be updated in update().
    void add_tuner(ITuner& tuner);

    //! Update renderer state.
    //! @remarks
    //!  Updates all added tuners.
    //! @returns
    //!  false if any tuner failed to update.
    virtual bool update();

    //! Attach all readers to sink.
    virtual void attach(ISink& sink);

    //! Detach all readers from sink.
    virtual void detach(ISink& sink);

private:
    static const size_t MaxChannels = ROC_CONFIG_MAX_CHANNELS;

    core::List<ITuner, core::NoOwnership> tuners_;
    core::Array<IStreamReader*, MaxChannels> readers_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_RENDERER_H_
