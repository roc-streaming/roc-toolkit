/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/latency_config.h"

namespace roc {
namespace audio {

void LatencyConfig::deduce_defaults(bool is_receiver) {
    if (fe_input == audio::FreqEstimatorInput_Default) {
        // On receiver, by default adjust clock based on NIQ latency.
        // On sender, by default do not adjust clock.
        fe_input = is_receiver ? audio::FreqEstimatorInput_NiqLatency
                               : audio::FreqEstimatorInput_Disable;
    }

    if (fe_profile == audio::FreqEstimatorProfile_Default) {
        fe_profile = target_latency < 30 * core::Millisecond
            // Prefer responsive profile on low latencies, because gradual profile
            // won't do it at all.
            ? audio::FreqEstimatorProfile_Responsive
            // Prefer gradual profile for higher latencies, because it can handle
            // higher network jitter.
            : audio::FreqEstimatorProfile_Gradual;
    }

    if (latency_tolerance < 0) {
        // This formula returns target_latency * N, where N starts with larger
        // number and approaches 0.5 as target_latency grows.
        // Examples:
        //  target=1ms -> tolerance=8ms (x8)
        //  target=10ms -> tolerance=20ms (x2)
        //  target=200ms -> tolerance=200ms (x1)
        //  target=2000ms -> tolerance=1444ms (x0.722)
        const core::nanoseconds_t capped_latency =
            std::max(target_latency, core::Millisecond);

        latency_tolerance = core::nanoseconds_t(
            capped_latency
            * (std::log((200 * core::Millisecond) * 2) / std::log(capped_latency * 2)));
    }
}

} // namespace audio
} // namespace roc
