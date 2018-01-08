/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/ireceiver.h
//! @brief Receiver pipeline interface.

#ifndef ROC_PIPELINE_IRECEIVER_H_
#define ROC_PIPELINE_IRECEIVER_H_

#include "roc_audio/frame.h"

namespace roc {
namespace pipeline {

//! Receiver pipeline interface.
class IReceiver {
public:
    virtual ~IReceiver();

    //! Receiver status.
    enum Status {
        //! Receiver is active.
        //! @remarks
        //!  There are connected clients and receiver returns samples from them.
        //!  Thought, there may be zero samples at the beginning and at the end
        //!  of the active period, for a duration of latency and timeout session
        //!  parameters.
        Active,

        //! Receiver is inactive.
        //! @remarks
        //!  There are no connected clients and receiver produces silence.
        Inactive
    };

    //! Get current receiver status.
    virtual Status status() const = 0;

    //! Wait until the receiver status becomes active.
    //! @remarks
    //!  Spurious wakeups are possible.
    virtual void wait_active() const = 0;

    //! Read frame.
    virtual void read(audio::Frame&) = 0;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_IRECEIVER_H_
