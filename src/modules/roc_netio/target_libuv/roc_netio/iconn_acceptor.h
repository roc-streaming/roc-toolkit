/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/iconn_acceptor.h
//! @brief TCP connection acceptor interface.

#ifndef ROC_NETIO_ICONN_ACCEPTOR_H_
#define ROC_NETIO_ICONN_ACCEPTOR_H_

#include "roc_netio/iconn_notifier.h"
#include "roc_netio/tcp_conn.h"

namespace roc {
namespace netio {

//! TCP connection acceptor interface.
class IConnAcceptor {
public:
    //! Destroy.
    virtual ~IConnAcceptor();

    //! accept() is called for every new incoming connection.
    //!
    //! @remarks
    //!  - Should be called from the event loop thread.
    //!
    //!  - It is a user responsibility to ensure that lifetime of the returned
    //!    object will be no less than lifetime of the provided TCP connection.
    //!
    //! @return
    //!  IConnNotifier object that will be notified when the provided connection
    //!  becomes readable or writeable.
    virtual IConnNotifier* accept(TCPConn&) = 0;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_ICONN_ACCEPTOR_H_
