/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/iterminate_handler.h
//! @brief Termination handler interface.

#ifndef ROC_NETIO_ITERMINATE_HANDLER_H_
#define ROC_NETIO_ITERMINATE_HANDLER_H_

#include "roc_netio/iconn.h"
#include "roc_netio/iconn_handler.h"

namespace roc {
namespace netio {

//! Termination handler interface.
class ITerminateHandler {
public:
    virtual ~ITerminateHandler();

    //! Handle completion of asynchronous termination of given connection.
    //! @note
    //!  This method is called from the network loop thread.
    virtual void handle_terminate_completed(IConn& conn, void* arg) = 0;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_ITERMINATE_HANDLER_H_
