/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/inetwork_task_completer.h
//! @brief Network task completion handler.

#ifndef ROC_NETIO_INETWORK_TASK_COMPLETER_H_
#define ROC_NETIO_INETWORK_TASK_COMPLETER_H_

#include "roc_netio/network_task.h"

namespace roc {
namespace netio {

//! Network task completion handler.
class INetworkTaskCompleter {
public:
    virtual ~INetworkTaskCompleter();

    //! Invoked when network task is completed.
    //! Should not block.
    virtual void network_task_completed(NetworkTask&) = 0;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_INETWORK_TASK_COMPLETER_H_
