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

#include "roc_core/atomic_bool.h"
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
//!
//! @remarks
//!  Signalling methods (register and unregister methods, set_broken(), set_closed())
//!  are lock-free only if core::Semaphore::post() is lock-free, which is true of
//!  target_posix_sem and target_darwin, but not of target_nosem. Besides, they never
//!  take locks when there are no waiters.
//!
//!  The tracker must outlive every call made on it, including wait_state() and
//!  signalling methods (register and unregister methods, set_broken(), set_closed()).
//!  These methods still touch internal members (mutex, condition variable,
//!  semaphore and its ownership flag) after their last observable state change.
//!  Hence, returning from wait_state() in one thread is not enough to safely
//!  destroy the tracker while other threads may still be inside a signalling
//!  call. The owner must ensure that all waiters and signallers have finished
//!  (e.g. by stopping network and control loops first) before destroying it.
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
    //!  BROKEN and CLOSED states are always matched, even if they're not in the
    //!  mask. They're terminal, so the state never changes after them, and
    //!  waiting for anything else would block until the deadline (or forever).
    //!
    //!  Empty mask means that there is nothing to wait for, and returns true
    //!  immediately.
    //!
    //!  Tracker itself never produces PAUSED state, and bits outside of
    //!  DeviceState are never matched, so such a wait ends only on BROKEN,
    //!  CLOSED, or deadline.
    //!
    //!  @p ret_states is set to the states that were matched, i.e. to the state
    //!  observed at the moment of the wakeup, masked with @p state_mask plus
    //!  BROKEN and CLOSED. It is set to zero if the deadline expired or if the
    //!  mask was empty.
    //!
    //!  Deadline is an absolute timestamp in ClockMonotonic domain:
    //!   - negative deadline means no deadline: blocks until the mask matches,
    //!     however long that takes;
    //!   - zero deadline, as well as any deadline that is already in the past,
    //!     means that state is checked once and the method returns immediately;
    //!   - positive deadline is the time point when waiting is given up.
    //!
    //!  Deadlines that are more than 10 years away from now are treated as
    //!  infinite, i.e. same as negative deadline.
    //!
    //! @returns
    //!  true if state matches the mask (or is BROKEN or CLOSED) and false if
    //!  deadline expired.
    //!
    //! @note
    //!  Remember that pipeline state may be outdated immediately after this
    //!  method returns (e.g. if new packet arrives concurrently), hence
    //!  @p ret_states reports what was matched, not what is current.
    bool
    wait_state(unsigned state_mask, unsigned& ret_states, core::nanoseconds_t deadline);

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
    core::AtomicInt<int32_t> activity_;
    core::AtomicBool sem_is_occupied_;
    core::Mutex mutex_;
    core::Cond cond_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_STATE_TRACKER_H_
