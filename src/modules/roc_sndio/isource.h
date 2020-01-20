/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/isource.h
//! @brief Source interface.

#ifndef ROC_SNDIO_ISOURCE_H_
#define ROC_SNDIO_ISOURCE_H_

#include "roc_audio/ireader.h"

namespace roc {
namespace sndio {

//! Source interface.
class ISource : public audio::IReader {
public:
    virtual ~ISource();

    //! Get source sample rate.
    virtual size_t sample_rate() const = 0;

    //! Get number of channels for the source.
    virtual size_t num_channels() const = 0;

    //! Check if the source has own clock.
    virtual bool has_clock() const = 0;

    //! Source state.
    enum State {
        //! Source is active and is producing some sound.
        Active,

        //! Source is inactive and is producing silence.
        Inactive,

        //! Source is explicitly paused.
        Paused
    };

    //! Get current source state.
    virtual State state() const = 0;

    //! Pause reading.
    virtual void pause() = 0;

    //! Resume paused reading.
    //! @returns
    //!  false if an error occured.
    virtual bool resume() = 0;

    //! Restart reading from the beginning.
    //! @remarks
    //!  If the reading is paused, it's automatically resumed.
    //! @returns
    //!  false if an error occured.
    virtual bool restart() = 0;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_ISOURCE_H_
