/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/isource.h
//! @brief Source interface.

#ifndef ROC_SNDIO_ISOURCE_H_
#define ROC_SNDIO_ISOURCE_H_

#include "roc_audio/iframe_reader.h"
#include "roc_core/time.h"
#include "roc_sndio/idevice.h"

namespace roc {
namespace sndio {

//! Source interface.
class ISource : virtual public IDevice, public audio::IFrameReader {
public:
    //! Initialize.
    explicit ISource(core::IArena& arena);

    //! Deinitialize.
    virtual ~ISource();

    //! Restart reading from beginning.
    //! @remarks
    //!  If the source has any sense of "beginning", this operation should rewind
    //!  reading to the beginning. Otherwise it can be no-op.
    virtual ROC_ATTR_NODISCARD status::StatusCode rewind() = 0;

    //! Adjust source clock to match consumer clock.
    //! @remarks
    //!  Invoked regularly after reading every or several frames.
    //!  @p timestamp defines the time in Unix domain when the last sample of the last
    //!  frame read from source is going to be actually processed by consumer.
    virtual void reclock(core::nanoseconds_t timestamp) = 0;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_ISOURCE_H_
