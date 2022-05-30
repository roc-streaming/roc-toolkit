/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_ctl/icontrol_task_completer.h
//! @brief Control task completion handler.

#ifndef ROC_CTL_ICONTROL_TASK_COMPLETER_H_
#define ROC_CTL_ICONTROL_TASK_COMPLETER_H_

#include "roc_ctl/control_task.h"

namespace roc {
namespace ctl {

//! Control task completion handler.
class IControlTaskCompleter {
public:
    virtual ~IControlTaskCompleter();

    //! Invoked when control task is completed.
    //! Should not block.
    virtual void control_task_completed(ControlTask&) = 0;
};

} // namespace ctl
} // namespace roc

#endif // ROC_CTL_ICONTROL_TASK_COMPLETER_H_
