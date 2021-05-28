/*
 * Copyright (c) 2021 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/operation_status.h
//! @brief Operation status codes.

#ifndef ROC_NETIO_OPERATION_STATUS_H_
#define ROC_NETIO_OPERATION_STATUS_H_

namespace roc {
namespace netio {

//! Asynchronous operation status.
enum AsyncOperationStatus {
    //! Operation is initiated and running.
    AsyncOp_Started,

    //! Operation is completed.
    AsyncOp_Completed
};

//! Socket operation status.
enum SocketOperationStatus {
    //! Operation can't be performed without blocking, try later.
    SocketOp_TryAgain = -1,

    //! End of stream, no more data.
    SocketOp_StreamEnd = -2,

    //! Stream failure.
    SocketOp_Failure = -3
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_OPERATION_STATUS_H_
