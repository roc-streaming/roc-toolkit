/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/iconn_acceptor.h
//! @brief Connection acceptor interface.

#ifndef ROC_NETIO_ICONN_ACCEPTOR_H_
#define ROC_NETIO_ICONN_ACCEPTOR_H_

#include "roc_netio/iconn.h"
#include "roc_netio/iconn_handler.h"

namespace roc {
namespace netio {

//! Connection acceptor interface.
//! @remarks
//!  - Methods are called from the network loop thread.
//!  - Methods should not block.
class IConnAcceptor {
public:
    virtual ~IConnAcceptor();

    //! Called for every new incoming connection.
    //!
    //! @returns
    //!  IConnHandler object that will be notified when the connection state
    //!  changes and when it becomes readable and writeable.
    //!
    //! @remarks
    //!  It is the caller responsibility to ensure that the handler is not
    //!  destroyed until remove_connection() call.
    virtual IConnHandler* add_connection(IConn&) = 0;

    //! Called after connection termination.
    //!
    //! @remarks
    //!  At this point, connection is already terminated and can't be used.
    //!  It's safe to destroy connection handler here.
    virtual void remove_connection(IConnHandler&) = 0;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_ICONN_ACCEPTOR_H_
