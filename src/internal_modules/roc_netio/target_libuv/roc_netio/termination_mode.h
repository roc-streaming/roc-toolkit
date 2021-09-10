/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/termination_mode.h
//! @brief Connection termination mode.

#ifndef ROC_NETIO_TERMINATION_MODE_H_
#define ROC_NETIO_TERMINATION_MODE_H_

namespace roc {
namespace netio {

//! Connection termination mode.
enum TerminationMode {
    //! Normal graceful termination.
    //! @remarks
    //!  Remote peer will recieve stream end without errors.
    Term_Normal,

    //! Termination with error.
    //! @remarks
    //!  Remote peer will receive connection reset and report failure.
    Term_Failure
};

//! Get string representation of termination mode.
const char* termination_mode_to_str(TerminationMode mode);

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_TERMINATION_MODE_H_
