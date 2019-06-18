/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/pump.h"
#include "roc_core/log.h"

namespace roc {
namespace sndio {

Pump::Pump(core::BufferPool<audio::sample_t>& buffer_pool,
           ISource& source,
           ISink& sink,
           size_t frame_size,
           Mode mode)
    : source_(source)
    , sink_(sink)
    , n_bufs_(0)
    , oneshot_(mode == ModeOneshot)
    , stop_(0) {
    if (buffer_pool.buffer_size() < frame_size) {
        roc_log(LogError, "pump: buffer size is too small: required=%lu actual=%lu",
                (unsigned long)frame_size, (unsigned long)buffer_pool.buffer_size());
        return;
    }

    frame_buffer_ = new (buffer_pool) core::Buffer<audio::sample_t>(buffer_pool);

    if (!frame_buffer_) {
        roc_log(LogError, "pump: can't allocate frame buffer");
        return;
    }

    frame_buffer_.resize(frame_size);
}

bool Pump::valid() const {
    return frame_buffer_;
}

bool Pump::run() {
    roc_log(LogDebug, "pump: starting main loop");

    while (!stop_) {
        if (source_.state() == ISource::Inactive) {
            if (oneshot_ && n_bufs_ != 0) {
                roc_log(LogInfo, "pump: got inactive status in oneshot mode");
                break;
            }
        } else {
            n_bufs_++;
        }

        audio::Frame frame(frame_buffer_.data(), frame_buffer_.size());
        if (!source_.read(frame)) {
            roc_log(LogDebug, "pump: got eof from source");
            break;
        }

        sink_.write(frame);
    }

    roc_log(LogDebug, "pump: exiting main loop, wrote %lu buffers",
            (unsigned long)n_bufs_);

    return !stop_;
}

void Pump::stop() {
    stop_ = 1;
}

} // namespace sndio
} // namespace roc
