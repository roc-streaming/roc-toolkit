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
    , sem_is_occupied_(0)
    , mutex_()
    , waiting_con_(mutex_) {
}

bool StateTracker::wait_state(unsigned state_mask, core::nanoseconds_t deadline) {
    if (state_mask == 0) {
        return true;
    }

    bool sem_owner = false;
    bool matched = false;

    mutex_.lock();
    while (true) {
        const core::nanoseconds_t now = core::timestamp(core::ClockMonotonic);
        const core::nanoseconds_t timeout = deadline - now;

        if (static_cast<unsigned>(get_state()) & state_mask) {
            matched = true;
            break;
        }

        if (deadline > 0 && timeout <= 0) {
            break;
        }

        if (!sem_owner) {
            sem_owner = sem_is_occupied_.compare_exchange(0, 1);
            if (sem_owner) {
                // Re-check state now that flag is published, otherwise a signal made
                // right before the flag was set would be lost.
                continue;
            }
        }

        if (sem_owner) {
            mutex_.unlock();
            if (deadline > 0) {
                (void)sem_.timed_wait(deadline);
            } else {
                sem_.wait();
            }
            mutex_.lock();
            waiting_con_.broadcast();
        } else {
            if (deadline > 0) {
                // Unlike Semaphore, Cond expects relative timeout.
                (void)waiting_con_.timed_wait(timeout);
            } else {
                waiting_con_.wait();
            }
        }
    }

    if (sem_owner) {
        // Hand semaphore ownership over to one of the condvar waiters.
        sem_is_occupied_ = 0;
        waiting_con_.broadcast();
    }

    mutex_.unlock();

    return matched;
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
    signal_state_change_();
}

void StateTracker::set_closed() {
    halt_state_ = sndio::DeviceState_Closed;
    signal_state_change_();
}

size_t StateTracker::num_sessions() const {
    return (size_t)active_sessions_;
}

void StateTracker::register_session() {
    if (active_sessions_++ == 0) {
        signal_state_change_();
    }
}

void StateTracker::unregister_session() {
    int prev_sessions = active_sessions_--;
    if (prev_sessions == 0) {
        roc_panic("state tracker: unpaired register/unregister session");
    } else if (prev_sessions == 1 && pending_packets_ == 0) {
        signal_state_change_();
    }
}

void StateTracker::register_packet() {
    if (pending_packets_++ == 0 && active_sessions_ == 0) {
        signal_state_change_();
    }
}

void StateTracker::unregister_packet() {
    int prev_packets = pending_packets_--;
    if (prev_packets == 0) {
        roc_panic("state tracker: unpaired register/unregister packet");
    } else if (prev_packets == 1 && active_sessions_ == 0) {
        signal_state_change_();
    }
}

void StateTracker::signal_state_change_() {
    if (sem_is_occupied_) {
        sem_.post();
    }
}

} // namespace pipeline
} // namespace roc
