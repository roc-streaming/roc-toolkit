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

#include "roc_audio/iwriter.h"
#include "roc_sndio/terminal.h"

namespace roc {
namespace sndio {

//! Sink interface.
class ISink : public ITerminal, public audio::IWriter {
public:
    virtual ~ISink();
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_ISINK_H_
