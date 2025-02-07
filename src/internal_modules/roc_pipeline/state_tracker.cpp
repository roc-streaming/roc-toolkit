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
    : sem_(0)
    , halt_state_(-1)
    , active_sessions_(0)
    , pending_packets_(0)
    , waiting_mask_(0) {
}

// This method should block until the state becomes any of the states specified by the
// mask, or deadline expires. E.g. if mask is ACTIVE | PAUSED, it should block until state
// becomes either ACTIVE or PAUSED. (Currently only two states are used, but later more
// states will be needed). Deadline should be an absolute timestamp.

// Questions:
// - When should the function return true vs false
bool StateTracker::wait_state(unsigned state_mask, core::nanoseconds_t deadline) {
    waiting_mask_ = state_mask;
    for (;;) {
        // If no state is specified in state_mask, return immediately
        if (state_mask == 0) {
            return true;
        }

        if (static_cast<unsigned>(get_state()) & state_mask) {
            waiting_mask_ = 0;
            return true;
        }

        if (deadline >= 0 && deadline <= core::timestamp(core::ClockMonotonic)) {
            waiting_mask_ = 0;
            return false;
        }

        if (deadline >= 0) {
            if (!sem_.timed_wait(deadline)) {
                waiting_mask_ = 0;
                return false;
            }
        } else {
            sem_.wait();
        }
    }
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
    if (active_sessions_++ == 0) {
        signal_state_change();
    }
}

void StateTracker::unregister_session() {
    int prev_sessions = active_sessions_--;
    if (prev_sessions == 0) {
        roc_panic("state tracker: unpaired register/unregister session");
    } else if (prev_sessions == 1 && pending_packets_ == 0) {
        signal_state_change();
    }

    // if (--active_sessions_ < 0) {
    //     roc_panic("state tracker: unpaired register/unregister session");
    // }
}

void StateTracker::register_packet() {
    if (pending_packets_++ == 0 && active_sessions_ == 0) {
        signal_state_change();
    }
}

void StateTracker::unregister_packet() {
    int prev_packets = pending_packets_--;
    if (prev_packets == 0) {
        roc_panic("state tracker: unpaired register/unregister packet");
    } else if (prev_packets == 1 && active_sessions_ == 0) {
        signal_state_change();
    }

    // if (--pending_packets_ < 0) {
    //     roc_panic("state tracker: unpaired register/unregister packet");
    // }
}

void StateTracker::signal_state_change() {
    if (waiting_mask_ != 0 && (static_cast<unsigned>(get_state()) & waiting_mask_)) {
        sem_.post();
    }
}

} // namespace pipeline
} // namespace roc
