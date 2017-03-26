/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/log.h"
#include "roc_audio/sample_buffer_queue.h"

namespace roc {
namespace audio {

SampleBufferQueue::SampleBufferQueue()
    : rd_sem_(0)
    , wr_sem_(MaxBuffers) {
    roc_log(LogDebug, "sample buffer queue: max_size=%u", (unsigned)MaxBuffers);
}

ISampleBufferConstSlice SampleBufferQueue::read() {
    rd_sem_.pend();

    ISampleBufferConstSlice buffer;

    {
        core::SpinMutex::Lock lock(mutex_);

        buffer = cb_.shift();
    }

    wr_sem_.post();

    return buffer;
}

void SampleBufferQueue::write(const ISampleBufferConstSlice& buffer) {
    wr_sem_.pend();

    {
        core::SpinMutex::Lock lock(mutex_);

        cb_.push(buffer);
    }

    rd_sem_.post();
}

size_t SampleBufferQueue::size() const {
    core::SpinMutex::Lock lock(mutex_);

    return cb_.size();
}

} // namespace audio
} // namespace roc
