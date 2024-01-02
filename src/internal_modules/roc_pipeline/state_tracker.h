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
#include "roc_core/stddefs.h"
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

    //! Compute current state.
    sndio::DeviceState get_state() const;

    //! Get active sessions counter.
    size_t num_active_sessions() const;

    //! Add/subtract to active sessions counter.
    void add_active_sessions(int increment);

    //! Get pending packets counter.
    size_t num_pending_packets() const;

    //! Add/subtract to pending packets counter.
    void add_pending_packets(int increment);

private:
    core::Atomic<int> active_sessions_;
    core::Atomic<int> pending_packets_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_STATE_TRACKER_H_
