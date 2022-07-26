/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/terminal_type.h
//! @brief Terminal type.

#ifndef ROC_SNDIO_TERMINAL_TYPE_H_
#define ROC_SNDIO_TERMINAL_TYPE_H_

namespace roc {
namespace sndio {

//! Terminal type.
enum TerminalType {
    Terminal_Sink,  //!< Sink.
    Terminal_Source //!< Source.
};

//! Convert terminal type to string.
const char* terminal_type_to_str(TerminalType type);

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_TERMINAL_TYPE_H_
