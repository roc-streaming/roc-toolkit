/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/latency_tuner.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

namespace {

const core::nanoseconds_t LogInterval = 5 * core::Second;

} // namespace

void LatencyConfig::deduce_defaults(core::nanoseconds_t default_target_latency,
                                    bool is_receiver) {
    if (target_latency < 0) {
        if (is_receiver) {
            if (tuner_profile != LatencyTunerProfile_Intact) {
                // Latency tuning is enabled on receiver.
                // Use default if target latency is not specified.
                target_latency = default_target_latency;
            } else {
                // Latency tuning is disabled on receiver.
                // Most likely, it is enabled on sender. To make tuning work on sender,
                // user should configure exactly same target latency on both sides.
                // To make this requirement clear, in this case we don't use any default
                // and require target latency to be specified explicitly on both sides.
                target_latency = 0;
            }
        } else {
            // See comment above.
            // If latency tuning is enabled on sender, we don't use any default
            // and require target latency to be specified explicitly.
            target_latency = 0;
        }
    }

    if (latency_tolerance < 0) {
        if (target_latency != 0) {
            // This formula returns target_latency * N, where N starts with larger
            // number and approaches 0.5 as target_latency grows.
            // Examples (for multiplier = 1):
            //  target=1ms -> tolerance=8ms (x8)
            //  target=10ms -> tolerance=20ms (x2)
            //  target=200ms -> tolerance=200ms (x1)
            //  target=2000ms -> tolerance=1444ms (x0.722)
            const core::nanoseconds_t capped_latency =
                std::max(target_latency, core::Millisecond);

            // On sender, apply multiplier to make default tolerance a bit higher than
            // on receiver. This way, if tolerance is enabled on both sides, receiver
            // will always trigger first.
            const int multiplier = is_receiver ? 1 : 2;

            latency_tolerance = core::nanoseconds_t(
                capped_latency
                * (std::log((200 * core::Millisecond) * 2 * multiplier)
                   / std::log(capped_latency * 2)));
        } else {
            // If default is not deduced for target latency, then it's also
            // not deduced for latency tolerance.
            latency_tolerance = 0;
        }
    }

    if (stale_tolerance < 0) {
        if (latency_tolerance != 0) {
            // Consider queue "stalling" if at least 1/4 of the missing latency
            // is caused by lack of new packets.
            stale_tolerance = latency_tolerance / 4;
        } else {
            // If default is not deduced for latency tolerance, then it's also
            // not deduced for stale tolerance.
            stale_tolerance = 0;
        }
    }

    if (tuner_backend == LatencyTunerBackend_Default) {
        tuner_backend = LatencyTunerBackend_Niq;
    }

    if (tuner_profile == LatencyTunerProfile_Default) {
        if (is_receiver) {
            if (tuner_backend == LatencyTunerBackend_Niq) {
                // If latency is low, we assume network jitter is also low. In this
                // case we use responsive profile. Gradual profile could cause
                // oscillations comparable with the latency and break playback.
                //
                // If latency is high, we assume the jitter may be also high. In
                // this case use gradual profile because it can handle high jitter
                // mich better.
                tuner_profile = target_latency < 30 * core::Millisecond
                    ? LatencyTunerProfile_Responsive
                    : LatencyTunerProfile_Gradual;
            } else {
                // E2E backend is not affected by network jitter that much, so
                // we can just always use responsive profile.
                tuner_profile = LatencyTunerProfile_Responsive;
            }
        } else {
            // On sender, by default disable latency tuning.
            // Typically latency tuning is done on receiver.
            tuner_profile = LatencyTunerProfile_Intact;
        }
    }
}

LatencyTuner::LatencyTuner(const LatencyConfig& config, const SampleSpec& sample_spec)
    : stream_pos_(0)
    , update_interval_(0)
    , update_pos_(0)
    , report_interval_(0)
    , report_pos_(0)
    , freq_coeff_(0)
    , freq_coeff_max_delta_(config.scaling_tolerance)
    , backend_(config.tuner_backend)
    , profile_(config.tuner_profile)
    , enable_checking_(config.latency_tolerance > 0)
    , enable_tuning_(profile_ != audio::LatencyTunerProfile_Intact)
    , has_niq_latency_(false)
    , niq_latency_(0)
    , niq_stalling_(0)
    , has_e2e_latency_(false)
    , e2e_latency_(0)
    , has_jitter_(false)
    , jitter_(0)
    , target_latency_(0)
    , min_latency_(0)
    , max_latency_(0)
    , max_stalling_(0)
    , sample_spec_(sample_spec)
    , valid_(false) {
    roc_log(LogDebug,
            "latency tuner: initializing:"
            " target_latency=%ld(%.3fms) latency_tolerance=%ld(%.3fms)"
            " stale_tolerance=%ld(%.3fms)"
            " scaling_interval=%ld(%.3fms) scaling_tolerance=%f"
            " backend=%s profile=%s",
            (long)sample_spec_.ns_2_stream_timestamp_delta(config.target_latency),
            (double)config.target_latency / core::Millisecond,
            (long)sample_spec_.ns_2_stream_timestamp_delta(config.latency_tolerance),
            (double)config.latency_tolerance / core::Millisecond,
            (long)sample_spec_.ns_2_stream_timestamp_delta(config.stale_tolerance),
            (double)config.stale_tolerance / core::Millisecond,
            (long)sample_spec_.ns_2_stream_timestamp_delta(config.scaling_interval),
            (double)config.scaling_interval / core::Millisecond,
            (double)config.scaling_tolerance, latency_tuner_backend_to_str(backend_),
            latency_tuner_profile_to_str(profile_));

    if (config.scaling_interval <= 0) {
        roc_log(LogError,
                "latency tuner: invalid config: scaling_interval out of bounds");
        return;
    }

    update_interval_ = sample_spec_.ns_2_stream_timestamp(config.scaling_interval);
    report_interval_ = sample_spec_.ns_2_stream_timestamp(LogInterval);

    if (enable_checking_ || enable_tuning_) {
        target_latency_ = sample_spec_.ns_2_stream_timestamp_delta(config.target_latency);

        if (target_latency_ <= 0) {
            roc_log(
                LogError,
                "latency tuner: invalid config: target_latency not set or out of bounds");
            return;
        }

        if (enable_checking_) {
            min_latency_ = sample_spec_.ns_2_stream_timestamp_delta(
                config.target_latency - config.latency_tolerance);
            max_latency_ = sample_spec_.ns_2_stream_timestamp_delta(
                config.target_latency + config.latency_tolerance);
            max_stalling_ =
                sample_spec_.ns_2_stream_timestamp_delta(config.stale_tolerance);

            if (target_latency_ < min_latency_ || target_latency_ > max_latency_) {
                roc_log(LogError,
                        "latency tuner: invalid config: latency_tolerance out of bounds");
                return;
            }

            if (max_stalling_ < 0) {
                roc_log(
                    LogError,
                    "latency tuner: invalid config: stalling_tolerance out of bounds");
                return;
            }
        }

        if (enable_tuning_) {
            if (freq_coeff_max_delta_ <= 0 || freq_coeff_max_delta_ >= 1) {
                roc_log(LogError,
                        "latency tuner: invalid config: scaling_tolerance out of bounds");
                return;
            }

            if (update_interval_ <= 0) {
                roc_log(LogError,
                        "latency tuner: invalid config: scaling_interval out of bounds");
                return;
            }

            fe_.reset(new (fe_)
                          FreqEstimator(profile_ == LatencyTunerProfile_Responsive
                                            ? FreqEstimatorProfile_Responsive
                                            : FreqEstimatorProfile_Gradual,
                                        (packet::stream_timestamp_t)target_latency_));
            if (!fe_) {
                return;
            }
        }
    }

    valid_ = true;
}

bool LatencyTuner::is_valid() const {
    return valid_;
}

void LatencyTuner::write_metrics(const LatencyMetrics& metrics) {
    roc_panic_if(!is_valid());

    if (metrics.niq_latency > 0 || metrics.niq_stalling > 0 || has_niq_latency_) {
        niq_latency_ = sample_spec_.ns_2_stream_timestamp_delta(metrics.niq_latency);
        niq_stalling_ = sample_spec_.ns_2_stream_timestamp_delta(metrics.niq_stalling);
        has_niq_latency_ = true;
    }

    if (metrics.e2e_latency > 0 || has_e2e_latency_) {
        e2e_latency_ = sample_spec_.ns_2_stream_timestamp_delta(metrics.e2e_latency);
        has_e2e_latency_ = true;
    }

    if (metrics.jitter > 0 || has_jitter_) {
        jitter_ = sample_spec_.ns_2_stream_timestamp_delta(metrics.jitter);
        has_jitter_ = true;
    }
}

bool LatencyTuner::update_stream() {
    roc_panic_if(!is_valid());

    packet::stream_timestamp_diff_t latency = 0;

    switch (backend_) {
    case audio::LatencyTunerBackend_Niq:
        if (!has_niq_latency_) {
            return true;
        }
        latency = niq_latency_;
        break;

    case audio::LatencyTunerBackend_E2e:
        if (!has_e2e_latency_) {
            return true;
        }
        latency = e2e_latency_;
        break;

    default:
        break;
    }

    if (enable_checking_) {
        if (!check_bounds_(latency)) {
            return false;
        }
    }

    if (enable_tuning_) {
        compute_scaling_(latency);
    }

    return true;
}

void LatencyTuner::advance_stream(packet::stream_timestamp_t duration) {
    roc_panic_if(!is_valid());

    stream_pos_ += duration;

    report_();
}

float LatencyTuner::get_scaling() const {
    roc_panic_if(!is_valid());

    return freq_coeff_;
}

bool LatencyTuner::check_bounds_(const packet::stream_timestamp_diff_t latency) {
    // Queue is considered "stalling" if there were no new packets for
    // some period of time.
    const bool is_stalling = backend_ == audio::LatencyTunerBackend_Niq
        && niq_stalling_ > max_stalling_ && max_stalling_ > 0;

    if (latency < min_latency_ && is_stalling) {
        // There are two possible reasons why queue latency becomes lower than mininmum:
        //  1. either we were not able to compensate clock drift (or compensation is
        //     disabled) and queue slowly exhausted,
        //  2. or there is a burst packet delay or drop, which led to sharp decrease
        //     of the latency.
        //
        // In the first case we normally want to terminate/restart session, but the
        // second case is often not a big deal. If the burst is short, pipeline
        // can easily recover from it, and terminating session would be worse.
        // In this case, we want to keep things as is and leave decision to the
        // watchdog. If the burst was short, watchdog will keep session, otherwise
        // no_playback_timeout will trigger and watchdog will terminate session.
        //
        // To distinguish between the cases, we check network queue stalling metric,
        // which shows delay since last received packet. If there were no packets
        // during notabe amount of time, we assume that the second case takes place.
        return true;
    }

    if (latency < min_latency_ || latency > max_latency_) {
        roc_log(
            LogDebug,
            "latency tuner: latency out of bounds:"
            " latency=%ld(%.3fms) target=%ld(%.3fms)"
            " min=%ld(%.3fms) max=%ld(%.3fms) stalling=%ld(%.3fms)",
            (long)latency, sample_spec_.stream_timestamp_delta_2_ms(latency),
            (long)target_latency_,
            sample_spec_.stream_timestamp_delta_2_ms(target_latency_), (long)min_latency_,
            sample_spec_.stream_timestamp_delta_2_ms(min_latency_), (long)max_latency_,
            sample_spec_.stream_timestamp_delta_2_ms(max_latency_), (long)niq_stalling_,
            sample_spec_.stream_timestamp_delta_2_ms(niq_stalling_));
        return false;
    }

    return true;
}

void LatencyTuner::compute_scaling_(packet::stream_timestamp_diff_t latency) {
    if (latency < 0) {
        latency = 0;
    }

    if (stream_pos_ < update_pos_) {
        return;
    }

    while (stream_pos_ >= update_pos_) {
        fe_->update((packet::stream_timestamp_t)latency);
        update_pos_ += update_interval_;
    }

    freq_coeff_ = fe_->freq_coeff();
    freq_coeff_ = std::min(freq_coeff_, 1.0f + freq_coeff_max_delta_);
    freq_coeff_ = std::max(freq_coeff_, 1.0f - freq_coeff_max_delta_);
}

void LatencyTuner::report_() {
    if (stream_pos_ < report_pos_) {
        return;
    }

    while (stream_pos_ >= report_pos_) {
        report_pos_ += report_interval_;
    }

    roc_log(
        LogDebug,
        "latency tuner:"
        " e2e_latency=%ld(%.3fms) niq_latency=%ld(%.3fms) target_latency=%ld(%.3fms)"
        " jitter=%ld(%.3fms) stale=%ld(%.3fms)"
        " fe=%.6f eff_fe=%.6f",
        (long)e2e_latency_, sample_spec_.stream_timestamp_delta_2_ms(e2e_latency_),
        (long)niq_latency_, sample_spec_.stream_timestamp_delta_2_ms(niq_latency_),
        (long)target_latency_, sample_spec_.stream_timestamp_delta_2_ms(target_latency_),
        (long)jitter_, sample_spec_.stream_timestamp_delta_2_ms(jitter_),
        (long)niq_stalling_, sample_spec_.stream_timestamp_delta_2_ms(niq_stalling_),
        (double)(fe_ && freq_coeff_ > 0 ? fe_->freq_coeff() : 0), (double)freq_coeff_);
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
