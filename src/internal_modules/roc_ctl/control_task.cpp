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

    flags_ = FlagDestroyed;
}

bool ControlTask::completed() const {
    return state_ == StateCompleted;
}

bool ControlTask::succeeded() const {
    const unsigned task_flags = flags_;

    validate_flags(task_flags);

    return task_flags & FlagSucceeded;
}

bool ControlTask::cancelled() const {
    const unsigned task_flags = flags_;

    validate_flags(task_flags);

    return task_flags & FlagCancelled;
}

void ControlTask::validate_flags(unsigned task_flags) {
    roc_panic_if_msg(
        task_flags & FlagDestroyed,
        "control task: detected corrupted task: FlagDestroyed is set: flags=0x%x",
        task_flags);

    int n_flags = 0;

    n_flags += !!(task_flags & FlagSucceeded);
    n_flags += !!(task_flags & FlagCancelled);
    n_flags += !!(task_flags & FlagPaused);

    roc_panic_if_msg(
        !(n_flags <= 1),
        "control task: detected corrupted task:"
        " FlagSucceeded, FlagCancelled, FlagPaused are mutually exclusive: flags=0x%x",
        task_flags);
}

void ControlTask::validate_deadline(core::nanoseconds_t deadline,
                                    core::seqlock_version_t version) {
    roc_panic_if_msg(!(deadline >= 0 || deadline == -1),
                     "control task: detected corrupted task: invalid deadline");

    roc_panic_if_msg(!core::seqlock_version_is_valid(version),
                     "control task: detected corrupted task: invalid version");
}

} // namespace ctl
} // namespace roc
