/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/iconn_notifier.h
//! @brief TCP connection notifier interface.

#ifndef ROC_NETIO_ICONN_NOTIFIER_H_
#define ROC_NETIO_ICONN_NOTIFIER_H_

namespace roc {
namespace netio {

//! TCP connection notifier.
class IConnNotifier {
public:
    //! Destroy.
    virtual ~IConnNotifier();

    //! Is notified when TCP connection becomes connected.
    virtual void notify_connected(bool connected) = 0;

    //! Is notified when TCP connection becomes writable.
    virtual void notify_writable(bool written) = 0;

    //! Is notified when TCP connection becomes readable.
    virtual void notify_readable() = 0;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_ICONN_NOTIFIER_H_
