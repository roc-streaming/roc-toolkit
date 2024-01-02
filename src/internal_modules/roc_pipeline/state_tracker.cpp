/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/state_tracker.h"
#include "roc_core/panic.h"

namespace roc {
namespace pipeline {

StateTracker::StateTracker()
    : active_sessions_(0)
    , pending_packets_(0) {
}

sndio::DeviceState StateTracker::get_state() const {
    if (active_sessions_ != 0) {
        // We have sessions and they're producing some sound.
        return sndio::DeviceState_Active;
    }

    if (pending_packets_ != 0) {
        // We don't have sessions, but we have packets that may create sessions.
        return sndio::DeviceState_Active;
    }

    // No sessions and packets; we can sleep until there are some.
    return sndio::DeviceState_Idle;
}

size_t StateTracker::num_active_sessions() const {
    return (size_t)active_sessions_;
}

void StateTracker::add_active_sessions(int increment) {
    const long result = active_sessions_ += increment;
    roc_panic_if(result < 0);
}

size_t StateTracker::num_pending_packets() const {
    return (size_t)pending_packets_;
}

void StateTracker::add_pending_packets(int increment) {
    const long result = pending_packets_ += increment;
    roc_panic_if(result < 0);
}

} // namespace pipeline
} // namespace roc
