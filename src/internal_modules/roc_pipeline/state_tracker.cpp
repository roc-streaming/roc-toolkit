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
    : halt_state_(-1)
    , active_sessions_(0)
    , pending_packets_(0) {
}

sndio::DeviceState StateTracker::get_state() const {
    const int halt_state = halt_state_;
    if (halt_state != -1) {
        // Happens if set_broken() or set_closed() was called.
        return (sndio::DeviceState)halt_state;
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

bool StateTracker::is_usable() const {
    const int halt_state = halt_state_;

    return halt_state != sndio::DeviceState_Broken
        && halt_state != sndio::DeviceState_Closed;
}

bool StateTracker::is_closed() const {
    const int halt_state = halt_state_;

    return halt_state == sndio::DeviceState_Closed;
}

void StateTracker::set_broken() {
    halt_state_ = sndio::DeviceState_Broken;
}

void StateTracker::set_closed() {
    halt_state_ = sndio::DeviceState_Closed;
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
