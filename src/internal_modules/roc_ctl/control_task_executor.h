/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_ctl/control_task_executor.h
//! @brief Control task executor.

#ifndef ROC_CTL_CONTROL_TASK_EXECUTOR_H_
#define ROC_CTL_CONTROL_TASK_EXECUTOR_H_

#include "roc_ctl/control_task.h"

namespace roc {
namespace ctl {

//! Control task executor interface.
//! @see ControlTaskExecutor.
class IControlTaskExecutor {
public:
    virtual ~IControlTaskExecutor();

private:
    friend class ControlTaskQueue;

    //! Execute task function.
    virtual ControlTaskResult execute_task(ControlTask& task,
                                           ControlTaskFunc task_func) = 0;
};

//! Control task executor.
//! @tparam E is the derived class.
//! @remarks
//!  If a class E wants to be capable of implementing its own tasks, it should
//!  inherit from ControlTaskExecutor<E>. This will enable the control queue
//!  to invoke tasks implemented as methods of E.
template <class E> class ControlTaskExecutor : public IControlTaskExecutor {
private:
    virtual ControlTaskResult execute_task(ControlTask& task, ControlTaskFunc task_func) {
        // We cast ourselves to derived class, cast task function to derived
        // class method, and invoke it.
        return (static_cast<E*>(this)
                    ->*(reinterpret_cast<ControlTaskResult (E::*)(ControlTask&)>(
                        task_func)))(task);
    }
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_CONTROL_TASK_EXECUTOR_H_
