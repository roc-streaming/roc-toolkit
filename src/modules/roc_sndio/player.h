/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/player.h
//! @brief Audio player.

#ifndef ROC_SNDIO_PLAYER_H_
#define ROC_SNDIO_PLAYER_H_

#include "roc_audio/iwriter.h"
#include "roc_core/atomic.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/slice.h"
#include "roc_core/stddefs.h"
#include "roc_core/thread.h"
#include "roc_pipeline/ireceiver.h"

namespace roc {
namespace sndio {

//! Audio player.
//! @remarks
//!  Reads samples from receiver and writes them to audio writer.
class Player : private core::Thread {
public:
    //! Initialize.
    Player(core::BufferPool<audio::sample_t>& buffer_pool,
           pipeline::IReceiver& input,
           audio::IWriter& output,
           size_t frame_size,
           bool oneshot);

    virtual ~Player();

    //! Check if the object was successfulyl constructed.
    bool valid() const;

    //! Start reading samples in a separate thread.
    bool start();

    //! Stop thread.
    //! @remarks
    //!  Can be called from any thread.
    void stop();

    //! Wait until background thread finishes.
    //! @remarks
    //!  Should be called once.
    void join();

private:
    virtual void run();

    pipeline::IReceiver& input_;
    audio::IWriter& output_;

    core::Slice<audio::sample_t> frame_buffer_;

    size_t n_bufs_;
    const bool oneshot_;

    core::Atomic stop_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_PLAYER_H_
