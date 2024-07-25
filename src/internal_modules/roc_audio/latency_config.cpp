/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/latency_config.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

namespace {

LatencyTunerProfile deduce_tuner_profile(const LatencyTunerBackend tuner_backend,
                                         const core::nanoseconds_t target_latency,
                                         const core::nanoseconds_t start_latency,
                                         const bool is_adaptive,
                                         const bool is_receiver) {
    if (is_receiver) {
        if (tuner_backend == LatencyTunerBackend_Niq) {
            // Use start_latency in adaptive mode or target_latency in fixed mode.
            const core::nanoseconds_t configured_latency =
                is_adaptive ? start_latency : target_latency;

            // If latency is low, we assume network jitter is also low. In this
            // case we use responsive profile. Gradual profile could cause
            // oscillations comparable with the latency and break playback.
            //
            // If latency is high, we assume the jitter may be also high. In
            // this case use gradual profile because it can handle high jitter
            // much better.
            return configured_latency > 0 && configured_latency < 30 * core::Millisecond
                ? LatencyTunerProfile_Responsive
                : LatencyTunerProfile_Gradual;
        } else {
            // E2E backend is not affected by network jitter that much, so
            // we can just always use responsive profile.
            return LatencyTunerProfile_Responsive;
        }
    } else {
        // On sender, by default disable latency tuning.
        // Typically latency tuning is done on receiver.
        return LatencyTunerProfile_Intact;
    }
}

core::nanoseconds_t deduce_start_latency(const core::nanoseconds_t min_latency,
                                         const core::nanoseconds_t max_latency,
                                         const core::nanoseconds_t default_latency) {
    if (min_latency != 0 || max_latency != 0) {
        // If min and max latency are provided explicitly, start in the middle.
        return min_latency + (max_latency - min_latency) / 2;
    } else {
        // Otherwise start from default value.
        return default_latency;
    }
}

void deduce_min_max_latency(const core::nanoseconds_t start_latency,
                            core::nanoseconds_t& min_latency,
                            core::nanoseconds_t& max_latency) {
    // By default, allow wide range for latency tuning.
    min_latency = 0;
    max_latency = start_latency * 5;
}

core::nanoseconds_t deduce_latency_tolerance(const core::nanoseconds_t target_latency,
                                             const core::nanoseconds_t start_latency,
                                             const bool is_adaptive,
                                             const bool is_receiver) {
    // On sender, apply multiplier to make default tolerance a bit higher than
    // on receiver. This way, if bounding is enabled on both sides, receiver
    // will always trigger first.
    const int multiplier = is_receiver ? 1 : 4;

    // Use start_latency in adaptive mode or target_latency in fixed mode.
    const core::nanoseconds_t configured_latency =
        is_adaptive ? start_latency : target_latency;

    // Out formula doesn't work well on latencies close to zero.
    const core::nanoseconds_t floored_latency =
        std::max(configured_latency, core::Millisecond);

    // This formula returns latency * N, where N starts with larger
    // number and approaches 0.5 as latency grows.
    // By default we're very tolerant and allow rather big oscillations.
    // Examples (for multiplier = 1):
    //  latency=1ms -> tolerance=8ms (x8)
    //  latency=10ms -> tolerance=20ms (x2)
    //  latency=200ms -> tolerance=200ms (x1)
    //  latency=2000ms -> tolerance=1444ms (x0.722)
    return core::nanoseconds_t(floored_latency
                               * (std::log((200 * core::Millisecond) * 2 * multiplier)
                                  / std::log(floored_latency * 2)));
}

core::nanoseconds_t deduce_stale_tolerance(const core::nanoseconds_t latency_tolerance) {
    // Consider queue "stalling" if at least 1/4 of the missing latency
    // is caused by lack of new packets.
    return std::max(latency_tolerance / 4, 10 * core::Millisecond);
}

size_t deduce_sliding_window(const LatencyTunerProfile tuner_profile) {
    if (tuner_profile == audio::LatencyTunerProfile_Responsive) {
        // Responsive profile requires faster reactions to changes
        // of link characteristics.
        return 10000;
    } else {
        return 30000;
    }
}

bool validate_adaptive_latency(const core::nanoseconds_t target_latency,
                               const core::nanoseconds_t latency_tolerance,
                               const core::nanoseconds_t start_latency,
                               const core::nanoseconds_t min_latency,
                               const core::nanoseconds_t max_latency) {
    roc_panic_if(target_latency != 0);

    if (latency_tolerance < 0) {
        roc_log(LogError, "latency config: latency_tolerance must be >= 0");
        return false;
    }

    if (start_latency < 0) {
        roc_log(LogError, "latency config: start_latency must be >= 0");
        return false;
    }

    if (min_latency != 0 || max_latency != 0) {
        if (min_latency < 0 || max_latency < 0) {
            roc_log(LogError, "latency config: min_latency and max_latency must be >= 0");
            return false;
        }
        if (min_latency > max_latency) {
            roc_log(LogError, "latency config: min_latency must be <= max_latency");
            return false;
        }
        if (start_latency != 0
            && (start_latency < min_latency || start_latency > max_latency)) {
            roc_log(
                LogError,
                "latency config: start_latency must be in [min_latency; max_latency]");
            return false;
        }
    }

    return true;
}

bool validate_fixed_latency(const core::nanoseconds_t target_latency,
                            const core::nanoseconds_t latency_tolerance,
                            const core::nanoseconds_t start_latency,
                            const core::nanoseconds_t min_latency,
                            const core::nanoseconds_t max_latency) {
    roc_panic_if(target_latency == 0);

    if (target_latency < 0) {
        roc_log(LogError, "latency config: target_latency must be >= 0");
        return false;
    }

    if (latency_tolerance < 0) {
        roc_log(LogError, "latency config: latency_tolerance must be >= 0");
        return false;
    }

    if (start_latency != 0 || min_latency != 0 || max_latency != 0) {
        roc_log(LogError,
                "latency config: start_latency, min_latency, max_latency"
                " may be used only when adaptive latency is enabled"
                " (i.e. target_latency == 0)");
        return false;
    }

    return true;
}

bool validate_intact_latency(const core::nanoseconds_t target_latency,
                             const core::nanoseconds_t latency_tolerance,
                             const core::nanoseconds_t start_latency,
                             const core::nanoseconds_t min_latency,
                             const core::nanoseconds_t max_latency) {
    if (target_latency < 0) {
        roc_log(LogError, "latency config: target_latency must be >= 0");
        return false;
    }

    if (latency_tolerance < 0) {
        roc_log(LogError, "latency config: latency_tolerance must be >= 0");
        return false;
    }

    if (start_latency != 0 || min_latency != 0 || max_latency != 0) {
        roc_log(LogError,
                "latency config:"
                " start_latency, min_latency, max_latency"
                " may be used only when latency tuning is enabled"
                " (i.e. latency profile is not \"intact\")");
        return false;
    }

    return true;
}

} // namespace

bool LatencyConfig::deduce_defaults(core::nanoseconds_t default_latency,
                                    bool is_receiver) {
    // Adaptive latency mode.
    const bool is_adaptive = target_latency == 0;

    if (tuner_backend == LatencyTunerBackend_Default) {
        tuner_backend = LatencyTunerBackend_Niq;
    }

    if (tuner_profile == LatencyTunerProfile_Default) {
        tuner_profile = deduce_tuner_profile(tuner_backend, target_latency, start_latency,
                                             is_adaptive, is_receiver);
    }

    if (tuner_profile != LatencyTunerProfile_Intact) {
        // If latency tuning and bounds checking are enabled.
        if (is_adaptive) {
            if (!validate_adaptive_latency(target_latency, latency_tolerance,
                                           start_latency, min_latency, max_latency)) {
                return false;
            }

            if (start_latency == 0) {
                start_latency =
                    deduce_start_latency(min_latency, max_latency, default_latency);
            }

            if (min_latency == 0 && max_latency == 0) {
                deduce_min_max_latency(start_latency, min_latency, max_latency);
            }
        } else {
            if (!validate_fixed_latency(target_latency, latency_tolerance, start_latency,
                                        min_latency, max_latency)) {
                return false;
            }
        }

        if (latency_tolerance == 0) {
            latency_tolerance = deduce_latency_tolerance(target_latency, start_latency,
                                                         is_adaptive, is_receiver);
        }

        if (stale_tolerance == 0) {
            stale_tolerance = deduce_stale_tolerance(latency_tolerance);
        }
    } else {
        // If latency tuning is disabled.
        if (!validate_intact_latency(target_latency, latency_tolerance, start_latency,
                                     min_latency, max_latency)) {
            return false;
        }

        if (target_latency != 0) {
            // If bounds checking is enabled.
            if (latency_tolerance == 0) {
                latency_tolerance = deduce_latency_tolerance(
                    target_latency, start_latency, is_adaptive, is_receiver);
            }

            if (stale_tolerance == 0) {
                stale_tolerance = deduce_stale_tolerance(latency_tolerance);
            }
        }
    }

    if (sliding_window_length == 0) {
        sliding_window_length = deduce_sliding_window(tuner_profile);
    }

    return true;
}

const char* latency_tuner_backend_to_str(LatencyTunerBackend backend) {
    switch (backend) {
    case LatencyTunerBackend_Default:
        return "default";

    case LatencyTunerBackend_Niq:
        return "niq";

    case LatencyTunerBackend_E2e:
        return "e2e";
    }

    return "<invalid>";
}

const char* latency_tuner_profile_to_str(LatencyTunerProfile profile) {
    switch (profile) {
    case LatencyTunerProfile_Default:
        return "default";

    case LatencyTunerProfile_Intact:
        return "intact";

    case LatencyTunerProfile_Responsive:
        return "responsive";

    case LatencyTunerProfile_Gradual:
        return "gradual";
    }

    return "<invalid>";
}

} // namespace audio
} // namespace roc
