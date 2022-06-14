/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_ctl/control_task.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace ctl {

ControlTask::~ControlTask() {
    if (state_ != StateCompleted) {
        roc_panic("control task: attempt to destroy task before it's completed");
    }
}

bool ControlTask::completed() const {
    return state_ == StateCompleted;
}

bool ControlTask::succeeded() const {
    return flags_ & FlagSucceeded;
}

bool ControlTask::cancelled() const {
    return flags_ & FlagCancelled;
}

} // namespace ctl
} // namespace roc
