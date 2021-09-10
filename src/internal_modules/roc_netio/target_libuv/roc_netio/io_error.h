/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/io_error.h
//! @brief I/O error codes.

#ifndef ROC_NETIO_IO_ERROR_H_
#define ROC_NETIO_IO_ERROR_H_

namespace roc {
namespace netio {

//! I/O error codes.
enum IOError {
    //! Operation can't be performed without blocking, try later.
    IOErr_WouldBlock = -1,

    //! End of stream, no more data.
    IOErr_StreamEnd = -2,

    //! Failure.
    IOErr_Failure = -3
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_IO_ERROR_H_
