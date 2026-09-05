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

#include "roc_core/atomic_int.h"
#include "roc_core/cond.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/semaphore.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_sndio/device_defs.h"

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

    //! Wait for state change.
    //!
    //! @remarks
    //!  Blocks until the state becomes any of the states specified by the mask,
    //!  or deadline expires. E.g. if mask is ACTIVE | PAUSED, blocks until
    //!  state becomes either ACTIVE or PAUSED.
    //!
    //!  Empty mask means that there is nothing to wait for, and returns true
    //!  immediately.
    //!
    //!  Deadline should be an absolute timestamp in ClockMonotonic domain.
    //!  Non-positive deadline (zero or negative) means no deadline: blocks
    //!  until the mask matches, however long that takes.
    //!
    //! @returns
    //!  true if state matches the mask and false if deadline expired.
    //!
    //! @note
    //!  Remember that pipeline state may be outdated immediately after this
    //!  method returns (e.g. if new packet arrives concurrently).
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
    void signal_state_change_();

    core::Semaphore sem_;
    core::AtomicInt<int32_t> halt_state_;
    core::AtomicInt<int32_t> active_sessions_;
    core::AtomicInt<int32_t> pending_packets_;
    core::AtomicInt<int32_t> sem_is_occupied_;
    core::Mutex mutex_;
    core::Cond waiting_con_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_STATE_TRACKER_H_
