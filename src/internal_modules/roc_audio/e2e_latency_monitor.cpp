/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/e2e_latency_monitor.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

EndToEndLatencyMonitor::EndToEndLatencyMonitor(IFrameReader& reader)
    : reader_(reader)
    , ready_(false)
    , e2e_latency_(0) {
}

EndToEndLatencyMonitor::~EndToEndLatencyMonitor() {
}

bool EndToEndLatencyMonitor::read(Frame& frame) {
    const bool res = reader_.read(frame);
    if (!!frame.capture_timestamp()) {
        const core::nanoseconds_t cur_ts = core::timestamp(core::ClockUnix);
        ready_ = true;
        e2e_latency_ = cur_ts - frame.capture_timestamp();
    } else {
        ready_ = false;
    }

    return res;
}

bool EndToEndLatencyMonitor::has_latency() const {
    return ready_;
}

core::nanoseconds_t EndToEndLatencyMonitor::latency() const {
    return e2e_latency_;
}

} // namespace audio
} // namespace roc
