/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/state_tracker.h
//! @brief Pipeline state tracker.

#ifndef ROC_PIPELINE_STATE_TRACKER_H_
#define ROC_PIPELINE_STATE_TRACKER_H_

#include "roc_core/atomic.h"
#include "roc_core/noncopyable.h"
#include "roc_core/semaphore.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_sndio/device_state.h"

namespace roc {
namespace pipeline {

//! Pipeline state tracker.
//!
//! All sender/receiver slots, endpoints, and sessions pass state updates to
//! the tracker, so that the top-level source/sink can quickly check if
//! there is any ongoing activity currently.
//!
//! Thread-safe.
class StateTracker : public core::NonCopyable<> {
public:
    //! Initialize all counters to zero.
    StateTracker();

    //! Block until state becomes any of the ones specified by state_mask.
    bool wait_state(unsigned state_mask, core::nanoseconds_t deadline);

    //! Compute current state.
    sndio::DeviceState get_state() const;

    //! Returns true if device is not broken or closed.
    bool is_usable() const;

    //! Returns true if device is closed.
    bool is_closed() const;

    //! Mark sender/receiver as broken.
    void set_broken();

    //! Mark sender/receiver as closed.
    void set_closed();

    //! Get active sessions counter.
    size_t num_sessions() const;

    //! Increment active sessions counter.
    void register_session();

    //! Decrement active sessions counter.
    void unregister_session();

    //! Increment pending packets counter.
    void register_packet();

    //! Decrement pending packets counter.
    void unregister_packet();

private:
    core::Semaphore sem_;
    core::Atomic<int> halt_state_;
    core::Atomic<int> active_sessions_;
    core::Atomic<int> pending_packets_;
    core::Atomic<unsigned> waiting_mask_;
    void signal_state_change();
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_STATE_TRACKER_H_
