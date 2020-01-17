/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/pump.h
//! @brief Pump.

#ifndef ROC_SNDIO_PUMP_H_
#define ROC_SNDIO_PUMP_H_

#include "roc_core/atomic.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_sndio/isink.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace sndio {

//! Audio pump.
//! @remarks
//!  Reads frames from source and writes them to sink.
class Pump : public core::NonCopyable<> {
public:
    //! Pump mode.
    enum Mode {
        // Run until the source return EOF.
        ModePermanent = 0,

        // Run until the source return EOF or become inactive first time.
        ModeOneshot = 1
    };

    //! Initialize.
    Pump(core::BufferPool<audio::sample_t>& buffer_pool,
         ISource& source,
         ISource* backup_source,
         ISink& sink,
         size_t frame_size,
         Mode mode);

    //! Check if the object was successfulyl constructed.
    bool valid() const;

    //! Run the pump.
    //! @remarks
    //!  Run until the stop() is called or, if oneshot mode is enabled,
    //!  the source becomes inactive.
    bool run();

    //! Stop the pump.
    //! @remarks
    //!  May be called from any thread.
    void stop();

private:
    ISource& main_source_;
    ISource* backup_source_;
    ISink& sink_;

    core::Slice<audio::sample_t> frame_buffer_;

    size_t n_bufs_;
    const bool oneshot_;

    core::Atomic stop_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_PUMP_H_
