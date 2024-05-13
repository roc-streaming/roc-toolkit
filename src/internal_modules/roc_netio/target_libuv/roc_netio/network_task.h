/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/network_task.h
//! @brief Network task.

#ifndef ROC_NETIO_NETWORK_TASK_H_
#define ROC_NETIO_NETWORK_TASK_H_

#include "roc_core/atomic.h"
#include "roc_core/mpsc_queue_node.h"
#include "roc_core/optional.h"
#include "roc_core/semaphore.h"
#include "roc_core/shared_ptr.h"
#include "roc_netio/basic_port.h"

namespace roc {
namespace netio {

class NetworkLoop;
class INetworkTaskCompleter;

//! Base class for network loop tasks.
class NetworkTask : public core::MpscQueueNode<> {
public:
    ~NetworkTask();

    //! Check that the task finished and succeeded.
    bool success() const;

protected:
    friend class NetworkLoop;

    NetworkTask();

    //! Task state.
    enum State {
        StateInitialized,
        StatePending,
        StateClosingPort,
        StateFinishing,
        StateFinished
    };

    //! Task implementation method.
    void (NetworkLoop::*func_)(NetworkTask&);

    //! Task state, defines whether task is finished already.
    //! The task becomes immutable after setting state to Finished.
    core::Atomic<int> state_;

    //! Task result, defines wether finished task succeeded or failed.
    //! Makes sense only after setting state_ to Finished.
    //! This atomic should be assigned before setting state_ to Finished.
    core::Atomic<int> success_;

    core::SharedPtr<BasicPort> port_; //!< On which port the task operates.
    void* port_handle_;               //!< Port handle.

    INetworkTaskCompleter* completer_;    //!< Completion handler.
    core::Optional<core::Semaphore> sem_; //!< Completion semaphore.
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_NETWORK_TASK_H_
