/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/ipipeline_task_completer.h
//! @brief Pipeline task completion handler.

#ifndef ROC_PIPELINE_IPIPELINE_TASK_COMPLETER_H_
#define ROC_PIPELINE_IPIPELINE_TASK_COMPLETER_H_

#include "roc_pipeline/pipeline_task.h"

namespace roc {
namespace pipeline {

//! Pipeline task completion handler.
class IPipelineTaskCompleter {
public:
    virtual ~IPipelineTaskCompleter();

    //! Invoked when a task is completed.
    virtual void pipeline_task_completed(PipelineTask&) = 0;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_IPIPELINE_TASK_COMPLETER_H_
