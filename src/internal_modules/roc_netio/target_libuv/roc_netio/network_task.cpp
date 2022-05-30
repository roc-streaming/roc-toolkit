/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/network_task.h"

namespace roc {
namespace netio {

NetworkTask::NetworkTask()
    : func_(NULL)
    , state_(StateInitialized)
    , success_(false)
    , port_handle_(NULL)
    , completer_(NULL) {
}

NetworkTask::~NetworkTask() {
    if (state_ != StateFinished) {
        roc_panic("network loop: attemp to destroy task before it's finished");
    }
}

bool NetworkTask::success() const {
    return state_ == StateFinished && success_;
}

} // namespace netio
} // namespace roc
