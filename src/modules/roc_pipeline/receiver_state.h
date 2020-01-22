/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_state.h
//! @brief Receiver pipeline state.

#ifndef ROC_PIPELINE_RECEIVER_STATE_H_
#define ROC_PIPELINE_RECEIVER_STATE_H_

#include "roc_core/atomic.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace pipeline {

//! Receiver pipeline state.
//! Thread-safe.
class ReceiverState : public core::NonCopyable<> {
public:
    //! Initialize.
    ReceiverState();

    //! Check whether pending packets counter is non-zero.
    bool has_pending_packets() const;

    //! Add given number to pending packets counter.
    void add_pending_packets(long increment);

    //! Get sessions counter.
    size_t num_sessions() const;

    //! Add given number to sessions counter.
    void add_sessions(long increment);

private:
    core::Atomic pending_packets_;
    core::Atomic sessions_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_STATE_H_
