/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/profiling_writer.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

namespace {

const core::nanoseconds_t LogInterval = core::Second;

} // namespace

ProfilingWriter::ProfilingWriter(IWriter& writer,
                                 packet::channel_mask_t channels,
                                 size_t sample_rate)
    : writer_(writer)
    , rate_limiter_(LogInterval)
    , avg_speed_(0)
    , avg_len_(0)
    , sample_rate_(sample_rate)
    , num_channels_(packet::num_channels(channels)) {
    if (num_channels_ == 0) {
        roc_panic("profiling writer: n_channels is zero");
    }
    if (sample_rate_ == 0) {
        roc_panic("profiling writer: sample_rate is zero");
    }
}

void ProfilingWriter::write(Frame& frame) {
    if (frame.size() % num_channels_ != 0) {
        roc_panic("profiling writer: unexpected frame size");
    }

    const core::nanoseconds_t elapsed = write_(frame);

    const double speed = (double)frame.size() / num_channels_ / elapsed * core::Second;

    update_(speed);
}

core::nanoseconds_t ProfilingWriter::write_(Frame& frame) {
    const core::nanoseconds_t start = core::timestamp();

    writer_.write(frame);

    return core::timestamp() - start;
}

void ProfilingWriter::update_(double speed) {
    avg_len_ += 1;
    avg_speed_ += (speed - avg_speed_) / avg_len_;

    if (rate_limiter_.allow()) {
        roc_log(LogDebug, "profiling writer: %lu sample/sec (%.2f sec/sec)",
                (unsigned long)avg_speed_, avg_speed_ / sample_rate_);
    }
}

} // namespace audio
} // namespace roc
