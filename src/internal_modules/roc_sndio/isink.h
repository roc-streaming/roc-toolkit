/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/isink.h
//! @brief Sink interface.

#ifndef ROC_SNDIO_ISINK_H_
#define ROC_SNDIO_ISINK_H_

#include "roc_audio/iframe_writer.h"
#include "roc_sndio/idevice.h"
#include "roc_status/status_code.h"

namespace roc {
namespace sndio {

//! Sink interface.
class ISink : virtual public IDevice, public audio::IFrameWriter {
public:
    //! Initialize.
    explicit ISink(core::IArena& arena);

    //! Deinitialize.
    virtual ~ISink();

    //! Flush buffered data, if any.
    virtual ROC_ATTR_NODISCARD status::StatusCode flush() = 0;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_ISINK_H_
