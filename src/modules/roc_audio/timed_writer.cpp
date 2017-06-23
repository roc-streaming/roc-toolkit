/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

#include "roc_audio/timed_writer.h"

namespace roc {
namespace audio {

TimedWriter::TimedWriter(ISampleBufferWriter& output,
                         packet::channel_mask_t channels,
                         size_t rate)
    : output_(output)
    , rate_(rate * packet::num_channels(channels))
    , n_samples_(0)
    , start_ms_(0) {
    if (rate_ == 0) {
        roc_panic("attempting to create timed writer with zero rate");
    }
}

void TimedWriter::write(const ISampleBufferConstSlice& buffer) {
    if (buffer) {
        if (n_samples_ == 0) {
            start_ms_ = core::timestamp_ms();
        } else {
            const uint64_t sleep_ms = n_samples_ * 1000 / rate_;

            core::sleep_until_ms(start_ms_ + sleep_ms);
        }

        n_samples_ += buffer.size();
    }

    output_.write(buffer);
}

} // namespace audio
} // namespace roc
