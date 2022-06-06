/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/pipeline_task.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace pipeline {

PipelineTask::PipelineTask()
    : state_(StateNew)
    , success_(false)
    , completer_(NULL) {
}

PipelineTask::~PipelineTask() {
    if (state_ == StateScheduled) {
        roc_panic("pipeline task: attempt to destroy task before it's finished");
    }
}

bool PipelineTask::success() const {
    return state_ == StateFinished && success_;
}

} // namespace pipeline
} // namespace roc
