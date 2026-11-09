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

namespace {

// Deadlines further than this from now are treated as infinite. This keeps deadline
// arithmetic of the condvar and semaphore backends far away from overflow.
const core::nanoseconds_t MaxDeadlineHorizon = 10 * 365 * core::Day;

} // namespace

StateTracker::StateTracker()
    : sem_(0)
    , halt_state_(-1)
    , active_sessions_(0)
    , pending_packets_(0)
    , activity_(0)
    , sem_is_occupied_(false)
    , mutex_()
    , cond_(mutex_) {
}

bool StateTracker::wait_state(unsigned state_mask,
                              unsigned& ret_states,
                              core::nanoseconds_t deadline) {
    ret_states = 0;

    if (state_mask == 0) {
        return true;
    }

    // Halt states are terminal: nothing else can happen after them, so a waiter
    // that didn't ask for them would never wake up. Always report them.
    const unsigned wait_mask =
        state_mask | sndio::DeviceState_Broken | sndio::DeviceState_Closed;

    if (deadline >= 0
        && deadline - core::timestamp(core::ClockMonotonic) > MaxDeadlineHorizon) {
        deadline = -1;
    }

    // Negative deadline means no deadline. Zero or past deadline means that state
    // is checked once.
    const bool has_deadline = deadline >= 0;

    bool sem_owner = false;
    bool matched = false;

    mutex_.lock();
    while (true) {
        const unsigned states = static_cast<unsigned>(get_state()) & wait_mask;
        if (states != 0) {
            ret_states = states;
            matched = true;
            break;
        }

        core::nanoseconds_t timeout = 0;
        if (has_deadline) {
            timeout = deadline - core::timestamp(core::ClockMonotonic);
            if (timeout <= 0) {
                break;
            }
        }

        if (!sem_owner) {
            sem_owner = sem_is_occupied_.compare_exchange(false, true);
            if (sem_owner) {
                // Drop tokens posted while nobody was blocked on the semaphore, to
                // avoid empty wakeups. This is safe because we re-check state after
                // that, and every dropped token was posted after a state update,
                // which the re-check will observe.
                while (sem_.timed_wait(0)) {
                }
                // Re-check state now that flag is published, otherwise a signal made
                // right before the flag was set would be lost.
                continue;
            }
        }

        if (sem_owner) {
            mutex_.unlock();
            if (has_deadline) {
                (void)sem_.timed_wait(deadline);
            } else {
                sem_.wait();
            }
            mutex_.lock();
            cond_.broadcast();
        } else {
            if (has_deadline) {
                // Unlike Semaphore, Cond expects relative timeout.
                (void)cond_.timed_wait(timeout);
            } else {
                cond_.wait();
            }
        }
    }

    if (sem_owner) {
        // Hand semaphore ownership over to one of the condvar waiters.
        sem_is_occupied_ = false;
        cond_.broadcast();
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

    // We have either sessions that are producing some sound, or packets that may
    // create sessions. Both are counted by a single counter, so that handing over
    // from a packet to a session never looks like an idle moment to a reader.
    if (activity_ != 0) {
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
    ++active_sessions_;
    if (activity_++ == 0) {
        signal_state_change_();
    }
}

void StateTracker::unregister_session() {
    int prev_sessions = active_sessions_--;
    if (prev_sessions == 0) {
        roc_panic("state tracker: unpaired register/unregister session");
    }
    if (activity_-- == 1) {
        signal_state_change_();
    }
}

void StateTracker::register_packet() {
    ++pending_packets_;
    if (activity_++ == 0) {
        signal_state_change_();
    }
}

void StateTracker::unregister_packet() {
    int prev_packets = pending_packets_--;
    if (prev_packets == 0) {
        roc_panic("state tracker: unpaired register/unregister packet");
    }
    if (activity_-- == 1) {
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
