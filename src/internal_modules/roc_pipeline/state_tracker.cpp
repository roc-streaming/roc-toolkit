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
    if (is_broken_) {
        return sndio::DeviceState_Broken;
    }

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

bool StateTracker::is_broken() const {
    return is_broken_;
}

void StateTracker::set_broken() {
    is_broken_ = true;
}

size_t StateTracker::num_sessions() const {
    return (size_t)active_sessions_;
}

void StateTracker::register_session() {
    active_sessions_++;
}

void StateTracker::unregister_session() {
    if (--active_sessions_ < 0) {
        roc_panic("state tracker: unpaired register/unregister session");
    }
}

void StateTracker::register_packet() {
    pending_packets_++;
}

void StateTracker::unregister_packet() {
    if (--pending_packets_ < 0) {
        roc_panic("state tracker: unpaired register/unregister packet");
    }
}

} // namespace pipeline
} // namespace roc
