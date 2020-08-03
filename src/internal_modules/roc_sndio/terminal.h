/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/terminal.h
//! @brief Terminal interface.

#ifndef ROC_SNDIO_TERMINAL_H_
#define ROC_SNDIO_TERMINAL_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace sndio {

//! Terminal type.
enum TerminalType {
    Terminal_Sink,  //!< Sink.
    Terminal_Source //!< Source.
};

//! Base interface for sinks and sources.
class ITerminal {
public:
    virtual ~ITerminal();

    //! Get sample rate of the terminal.
    virtual size_t sample_rate() const = 0;

    //! Get number of channels for the terminal.
    virtual size_t num_channels() const = 0;

    //! Check if the terminal has own clock.
    virtual bool has_clock() const = 0;
};

//! Convert terminal type to string.
const char* terminal_type_to_str(TerminalType type);

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_TERMINAL_H_
