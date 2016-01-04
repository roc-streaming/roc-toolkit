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
class SampleBufferQueue : public ISampleBufferReader,
                          public ISampleBufferWriter,
                          public core::NonCopyable<> {
public:
    //! Construct empty queue.
    //! @remarks
    //!  - If @p max_size is zero, maximum possible size is used.
    //!  - If @p wait_full is true, first read() call will block until
    //!    queue becomes full first time.
    SampleBufferQueue(size_t max_size = ROC_CONFIG_DEFAULT_PLAYER_LATENCY,
                      bool wait_full = true);

    //! Read buffer.
    //! @remarks
    //!  Blocks until there is at least one buffer in queue. If
    //!  this is first read() call, blocks until there are at
    //!  least @p start_threshold buffers in queue.
    virtual ISampleBufferConstSlice read();

    //! Write buffer.
    //! @remarks
    //!  Blocks until there are less than MaxSz buffers in queue.
    virtual void write(const ISampleBufferConstSlice& buffer);

    //! Get current queue size.
    size_t size() const;

private:
    enum { MaxBuffers = ROC_CONFIG_MAX_SAMPLE_BUFFERS };

    const size_t max_size_;
    size_t countdown_;

    core::Semaphore rd_sem_;
    core::Semaphore wr_sem_;

    core::CircularBuffer<ISampleBufferConstSlice, MaxBuffers> cb_;
    core::SpinMutex mutex_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SAMPLE_BUFFER_QUEUE_H_
