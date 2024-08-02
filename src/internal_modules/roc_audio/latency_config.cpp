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
                                         const core::nanoseconds_t start_target_latency,
                                         const bool is_adaptive,
                                         const bool is_receiver) {
    if (is_receiver) {
        if (tuner_backend == LatencyTunerBackend_Niq) {
            const core::nanoseconds_t configured_latency =
                is_adaptive ? start_target_latency : target_latency;

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

core::nanoseconds_t
deduce_start_target_latency(const core::nanoseconds_t min_target_latency,
                            const core::nanoseconds_t max_target_latency,
                            const core::nanoseconds_t default_latency) {
    if (min_target_latency != 0 || max_target_latency != 0) {
        // If min and max latency are provided explicitly, start in the middle.
        return min_target_latency + (max_target_latency - min_target_latency) / 2;
    } else {
        // Otherwise start from default value.
        return default_latency;
    }
}

void deduce_min_max_target_latency(const core::nanoseconds_t start_target_latency,
                                   core::nanoseconds_t& min_target_latency,
                                   core::nanoseconds_t& max_target_latency) {
    // By default, allow wide range for latency tuning.
    min_target_latency = std::min(core::Millisecond * 15, start_target_latency / 5);
    max_target_latency = start_target_latency * 5;
}

core::nanoseconds_t
deduce_latency_tolerance(const core::nanoseconds_t target_latency,
                         const core::nanoseconds_t start_target_latency,
                         const bool is_adaptive,
                         const bool is_receiver) {
    // On sender, apply multiplier to make default tolerance a bit higher than
    // on receiver. This way, if bounding is enabled on both sides, receiver
    // will always trigger first.
    const int multiplier = is_receiver ? 1 : 4;

    const core::nanoseconds_t configured_latency =
        is_adaptive ? start_target_latency : target_latency;

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

bool validate_adaptive_latency(const core::nanoseconds_t target_latency,
                               const core::nanoseconds_t latency_tolerance,
                               const core::nanoseconds_t start_target_latency,
                               const core::nanoseconds_t min_target_latency,
                               const core::nanoseconds_t max_target_latency) {
    roc_panic_if(target_latency != 0);

    if (latency_tolerance < 0) {
        roc_log(LogError, "latency config: latency_tolerance must be >= 0");
        return false;
    }

    if (start_target_latency < 0) {
        roc_log(LogError, "latency config: start_target_latency must be >= 0");
        return false;
    }

    if (min_target_latency != 0 || max_target_latency != 0) {
        if (min_target_latency < 0 || max_target_latency < 0) {
            roc_log(
                LogError,
                "latency config: min_target_latency and max_target_latency must be >= 0");
            return false;
        }
        if (min_target_latency > max_target_latency) {
            roc_log(LogError,
                    "latency config: min_target_latency must be <= max_target_latency");
            return false;
        }
        if (start_target_latency != 0
            && (start_target_latency < min_target_latency
                || start_target_latency > max_target_latency)) {
            roc_log(LogError,
                    "latency config: start_target_latency must be in range"
                    " [min_target_latency; max_target_latency]");
            return false;
        }
    }

    return true;
}

bool validate_fixed_latency(const core::nanoseconds_t target_latency,
                            const core::nanoseconds_t latency_tolerance,
                            const core::nanoseconds_t start_target_latency,
                            const core::nanoseconds_t min_target_latency,
                            const core::nanoseconds_t max_target_latency) {
    roc_panic_if(target_latency == 0);

    if (target_latency < 0) {
        roc_log(LogError, "latency config: target_latency must be >= 0");
        return false;
    }

    if (latency_tolerance < 0) {
        roc_log(LogError, "latency config: latency_tolerance must be >= 0");
        return false;
    }

    if (start_target_latency != 0 || min_target_latency != 0 || max_target_latency != 0) {
        roc_log(
            LogError,
            "latency config: start_target_latency, min_target_latency, max_target_latency"
            " may be used only when adaptive latency is enabled"
            " (i.e. target_latency == 0)");
        return false;
    }

    return true;
}

bool validate_no_latency(const core::nanoseconds_t target_latency,
                         const core::nanoseconds_t latency_tolerance,
                         const core::nanoseconds_t start_target_latency,
                         const core::nanoseconds_t min_target_latency,
                         const core::nanoseconds_t max_target_latency) {
    if (target_latency != 0 || latency_tolerance != 0 || start_target_latency != 0
        || min_target_latency != 0 || max_target_latency != 0) {
        roc_log(LogError,
                "latency config:"
                " on sender, target_latency, latency_tolerance,"
                " start_target_latency, min_target_latency, max_target_latency"
                " aren't used and must be zero if latency profile is \"intact\"");
        return false;
    }

    return true;
}

} // namespace

bool LatencyConfig::deduce_defaults(core::nanoseconds_t default_latency,
                                    bool is_receiver) {
    // Whether we're using adaptive latency mode.
    const bool is_adaptive = target_latency == 0;

    if (tuner_backend == LatencyTunerBackend_Auto) {
        tuner_backend = LatencyTunerBackend_Niq;
    }

    if (tuner_profile == LatencyTunerProfile_Auto) {
        tuner_profile =
            deduce_tuner_profile(tuner_backend, target_latency, start_target_latency,
                                 is_adaptive, is_receiver);
    }

    // On receiver, we always need to know latency parameters, no matter who is doing
    // latency adjustment, receiver or sender.
    // On sender, we need latency parameters only if sender is doing latency adjustment
    // (latency profile is not "intact").
    const bool want_latency_params =
        is_receiver || tuner_profile != LatencyTunerProfile_Intact;

    if (want_latency_params) {
        if (is_adaptive) {
            if (!validate_adaptive_latency(target_latency, latency_tolerance,
                                           start_target_latency, min_target_latency,
                                           max_target_latency)) {
                return false;
            }

            if (start_target_latency == 0) {
                start_target_latency = deduce_start_target_latency(
                    min_target_latency, max_target_latency, default_latency);
            }

            if (min_target_latency == 0 && max_target_latency == 0) {
                deduce_min_max_target_latency(start_target_latency, min_target_latency,
                                              max_target_latency);
            }
        } else {
            if (!validate_fixed_latency(target_latency, latency_tolerance,
                                        start_target_latency, min_target_latency,
                                        max_target_latency)) {
                return false;
            }
        }

        if (latency_tolerance == 0) {
            latency_tolerance = deduce_latency_tolerance(
                target_latency, start_target_latency, is_adaptive, is_receiver);
        }

        if (stale_tolerance == 0) {
            stale_tolerance = deduce_stale_tolerance(latency_tolerance);
        }
    } else {
        if (!validate_no_latency(target_latency, latency_tolerance, start_target_latency,
                                 min_target_latency, max_target_latency)) {
            return false;
        }
    }

    return true;
}

const char* latency_tuner_backend_to_str(LatencyTunerBackend backend) {
    switch (backend) {
    case LatencyTunerBackend_Auto:
        return "auto";

    case LatencyTunerBackend_Niq:
        return "niq";

    case LatencyTunerBackend_E2e:
        return "e2e";
    }

    return "<invalid>";
}

const char* latency_tuner_profile_to_str(LatencyTunerProfile profile) {
    switch (profile) {
    case LatencyTunerProfile_Auto:
        return "auto";

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
