/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_state.h"
#include "roc_core/panic.h"

namespace roc {
namespace pipeline {

ReceiverState::ReceiverState()
    : pending_packets_(0)
    , sessions_(0) {
}

bool ReceiverState::has_pending_packets() const {
    return pending_packets_;
}

void ReceiverState::add_pending_packets(long increment) {
    const long result = pending_packets_ += increment;
    roc_panic_if(result < 0);
}

size_t ReceiverState::num_sessions() const {
    return (size_t)sessions_;
}

void ReceiverState::add_sessions(long increment) {
    const long result = sessions_ += increment;
    roc_panic_if(result < 0);
}

} // namespace pipeline
} // namespace roc
