/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_audio/delayed_writer.h"

namespace roc {
namespace audio {

DelayedWriter::DelayedWriter(ISampleBufferWriter& output,
                             packet::channel_mask_t channels,
                             size_t latency)
    : output_(output)
    , n_ch_(packet::num_channels(channels))
    , latency_(latency * n_ch_)
    , pending_(0)
    , flushed_(latency_ == 0) {
}

void DelayedWriter::write(const ISampleBufferConstSlice& buffer) {
    if (flushed_) {
        output_.write(buffer);
    } else {
        queue_.append(buffer);
        pending_ += buffer.size();

        if (pending_ >= latency_ || !buffer) {
            roc_log(LogDebug, "delayed writer: starting output: latency=%lu pending=%lu",
                    (unsigned long)latency_ / n_ch_, (unsigned long)pending_ / n_ch_);

            for (size_t n = 0; n < queue_.size(); n++) {
                output_.write(queue_[n]);
            }

            flushed_ = true;
        }
    }
}

} // namespace audio
} // namespace roc
