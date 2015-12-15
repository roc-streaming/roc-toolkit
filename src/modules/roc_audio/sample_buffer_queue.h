/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/sample_buffer_queue.h
//! @brief Sample buffer queue.

#ifndef ROC_AUDIO_SAMPLE_BUFFER_QUEUE_H_
#define ROC_AUDIO_SAMPLE_BUFFER_QUEUE_H_

#include "roc_config/config.h"

#include "roc_core/noncopyable.h"
#include "roc_core/semaphore.h"
#include "roc_core/spin_mutex.h"
#include "roc_core/circular_buffer.h"

#include "roc_audio/isample_buffer_reader.h"
#include "roc_audio/isample_buffer_writer.h"

namespace roc {
namespace audio {

//! Sample buffer queue.
//! @tparam MaxSz is maximum queue size; should be >= 1.
template <size_t MaxSz = ROC_CONFIG_DEFAULT_PLAYER_LATENCY>
class SampleBufferQueue : public ISampleBufferReader,
                          public ISampleBufferWriter,
                          public core::NonCopyable<> {
public:
    //! Construct empty queue.
    //! @remarks
    //!  First read() call will block until at least @p start_threshold
    //!  buffers are queued.
    SampleBufferQueue(size_t start_threshold = MaxSz)
        : rd_sem_(0)
        , wr_sem_(MaxSz)
        , countdown_(start_threshold) {
    }

    //! Read buffer.
    //! @remarks
    //!  Blocks until there is at least one buffer in queue. If
    //!  this is first read() call, blocks until there are at
    //!  least @p start_threshold buffers in queue.
    virtual ISampleBufferConstSlice read() {
        rd_sem_.pend();

        ISampleBufferConstSlice buffer;

        {
            core::SpinMutex::Lock lock(mutex_);

            buffer = cb_.shift();
        }

        wr_sem_.post();

        return buffer;
    }

    //! Write buffer.
    //! @remarks
    //!  Blocks until there are less than MaxSz buffers in queue.
    virtual void write(const ISampleBufferConstSlice& buffer) {
        wr_sem_.pend();

        bool post = false;

        {
            core::SpinMutex::Lock lock(mutex_);

            cb_.push(buffer);

            if (countdown_ != 0) {
                countdown_--;
            }

            post = (countdown_ == 0);
        }

        if (post) {
            rd_sem_.post();
        }
    }

    //! Get current queue size.
    size_t size() const {
        core::SpinMutex::Lock lock(mutex_);

        return cb_.size();
    }

private:
    core::Semaphore rd_sem_;
    core::Semaphore wr_sem_;

    core::CircularBuffer<ISampleBufferConstSlice, MaxSz> cb_;
    core::SpinMutex mutex_;

    size_t countdown_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SAMPLE_BUFFER_QUEUE_H_
