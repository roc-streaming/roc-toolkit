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

// Calculates latency decreasment step value such that
// if current latency equals exactly upper threshold value, after the decreasment
// it will get in the middle between threshold and estimated value.
float upper_coef_to_step_lat_update(const float x) {
    return ((x + 1.f) / (x * 2.f));
}

// Calculates latency increasment step value based on
// latency_decrease_relative_threshold.
float lower_thrs_to_step_lat_update(const float x) {
    return (x + 1.f) / 2.f;
}

} // namespace

LatencyTuner::LatencyTuner(const LatencyConfig& latency_config,
                           const FreqEstimatorConfig& fe_config,
                           const SampleSpec& sample_spec,
                           dbgio::CsvDumper* dumper)
    : stream_pos_(0)
    , scale_interval_(sample_spec.ns_2_stream_timestamp(latency_config.scaling_interval))
    , scale_pos_(0)
    , report_interval_(sample_spec.ns_2_stream_timestamp(LogInterval))
    , report_pos_(0)
    , has_new_freq_coeff_(false)
    , freq_coeff_(0)
    , freq_coeff_max_delta_(latency_config.scaling_tolerance)
    , backend_(latency_config.tuner_backend)
    , profile_(latency_config.tuner_profile)
    , enable_latency_adjustment_(latency_config.tuner_profile
                                 != LatencyTunerProfile_Intact)
    , enable_tolerance_checks_(latency_config.tuner_profile != LatencyTunerProfile_Intact
                               || latency_config.target_latency != 0
                               || latency_config.start_target_latency != 0)
    , latency_is_adaptive_(latency_config.target_latency == 0)
    , has_niq_latency_(false)
    , niq_latency_(0)
    , niq_stalling_(0)
    , has_e2e_latency_(false)
    , e2e_latency_(0)
    , has_metrics_(false)
    , cur_target_latency_(0)
    , min_target_latency_(0)
    , max_target_latency_(0)
    , min_actual_latency_(0)
    , max_actual_latency_(0)
    , max_stalling_(
          sample_spec.ns_2_stream_timestamp_delta(latency_config.stale_tolerance))
    , sample_spec_(sample_spec)
    , target_latency_state_(TL_STARTING)
    , starting_timeout_(
          sample_spec.ns_2_stream_timestamp_delta(latency_config.starting_timeout))
    , cooldown_dec_timeout_(
          sample_spec.ns_2_stream_timestamp_delta(latency_config.cooldown_dec_timeout))
    , cooldown_inc_timeout_(
          sample_spec.ns_2_stream_timestamp_delta(latency_config.cooldown_inc_timeout))
    , max_jitter_overhead_(latency_config.max_jitter_overhead)
    , mean_jitter_overhead_(latency_config.mean_jitter_overhead)
    , last_target_latency_update_(0)
    , lat_update_upper_thrsh_(latency_config.latency_decrease_relative_threshold)
    , lat_update_dec_step_(upper_coef_to_step_lat_update(
          latency_config.latency_decrease_relative_threshold))
    , lat_update_inc_step_(lower_thrs_to_step_lat_update(
          latency_config.latency_decrease_relative_threshold))
    , last_lat_limiter_(LogInterval)
    , dumper_(dumper)
    , init_status_(status::NoStatus) {
    roc_log(
        LogDebug,
        "latency tuner: initializing:"
        " backend=%s profile=%s tuning=%s"
        " target_latency=%ld(%.3fms) latency_tolerance=%ld(%.3fms)"
        " start_latency=%ld(%.3fms) min_latency=%ld(%.3fms) max_latency=%ld(%.3fms)"
        " stale_tolerance=%ld(%.3fms)"
        " scaling_interval=%ld(%.3fms) scaling_tolerance=%.3f",
        // backend, profile, tuning
        latency_tuner_backend_to_str(backend_), latency_tuner_profile_to_str(profile_),
        enable_latency_adjustment_ && latency_is_adaptive_ ? "adaptive"
            : enable_latency_adjustment_                   ? "fixed"
                                                           : "disabled",
        // target_latency, latency_tolerance
        (long)sample_spec_.ns_2_stream_timestamp_delta(latency_config.target_latency),
        (double)latency_config.target_latency / core::Millisecond,
        (long)sample_spec_.ns_2_stream_timestamp_delta(latency_config.latency_tolerance),
        (double)latency_config.latency_tolerance / core::Millisecond,
        // start_latency, min_latency, max_latency
        (long)sample_spec_.ns_2_stream_timestamp_delta(
            latency_config.start_target_latency),
        (double)latency_config.start_target_latency / core::Millisecond,
        (long)sample_spec_.ns_2_stream_timestamp_delta(latency_config.min_target_latency),
        (double)latency_config.min_target_latency / core::Millisecond,
        (long)sample_spec_.ns_2_stream_timestamp_delta(latency_config.max_target_latency),
        (double)latency_config.max_target_latency / core::Millisecond,
        // stale_tolerance
        (long)sample_spec_.ns_2_stream_timestamp_delta(latency_config.stale_tolerance),
        (double)latency_config.stale_tolerance / core::Millisecond,
        // scaling_interval, scaling_tolerance
        (long)sample_spec_.ns_2_stream_timestamp_delta(latency_config.scaling_interval),
        (double)latency_config.scaling_interval / core::Millisecond,
        (double)latency_config.scaling_tolerance);

    if (enable_latency_adjustment_ || enable_tolerance_checks_) {
        if (latency_is_adaptive_) {
            roc_panic_if_msg(latency_config.target_latency != 0
                                 || latency_config.start_target_latency <= 0
                                 || latency_config.min_target_latency < 0
                                 || latency_config.max_target_latency <= 0,
                             "latency tuner: invalid configuration");

            cur_target_latency_ = sample_spec_.ns_2_stream_timestamp_delta(
                latency_config.start_target_latency);

            min_target_latency_ = sample_spec_.ns_2_stream_timestamp_delta(
                latency_config.min_target_latency);
            max_target_latency_ = sample_spec_.ns_2_stream_timestamp_delta(
                latency_config.max_target_latency);

            min_actual_latency_ = sample_spec_.ns_2_stream_timestamp_delta(
                latency_config.min_target_latency - latency_config.latency_tolerance);
            max_actual_latency_ = sample_spec_.ns_2_stream_timestamp_delta(
                latency_config.max_target_latency + latency_config.latency_tolerance);
        } else {
            roc_panic_if_msg(latency_config.target_latency <= 0
                                 || latency_config.start_target_latency != 0
                                 || latency_config.min_target_latency != 0
                                 || latency_config.max_target_latency != 0,
                             "latency tuner: invalid configuration");

            cur_target_latency_ =
                sample_spec_.ns_2_stream_timestamp_delta(latency_config.target_latency);

            min_actual_latency_ = sample_spec_.ns_2_stream_timestamp_delta(
                latency_config.target_latency - latency_config.latency_tolerance);
            max_actual_latency_ = sample_spec_.ns_2_stream_timestamp_delta(
                latency_config.target_latency + latency_config.latency_tolerance);
        }

        if (enable_latency_adjustment_) {
            fe_.reset(new (fe_) FreqEstimator(
                fe_config, (packet::stream_timestamp_t)cur_target_latency_, sample_spec,
                dumper_));
        }
    }

    init_status_ = status::StatusOK;
}

status::StatusCode LatencyTuner::init_status() const {
    return init_status_;
}

void LatencyTuner::write_metrics(const LatencyMetrics& latency_metrics,
                                 const packet::LinkMetrics& link_metrics) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (latency_metrics.niq_latency > 0 || latency_metrics.niq_stalling > 0
        || has_niq_latency_) {
        niq_latency_ =
            sample_spec_.ns_2_stream_timestamp_delta(latency_metrics.niq_latency);
        niq_stalling_ =
            sample_spec_.ns_2_stream_timestamp_delta(latency_metrics.niq_stalling);
        has_niq_latency_ = true;
    }

    if (latency_metrics.e2e_latency > 0 || has_e2e_latency_) {
        e2e_latency_ =
            sample_spec_.ns_2_stream_timestamp_delta(latency_metrics.e2e_latency);
        has_e2e_latency_ = true;
    }

    latency_metrics_ = latency_metrics;
    link_metrics_ = link_metrics;

    has_metrics_ = true;
}

bool LatencyTuner::update_stream() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (enable_latency_adjustment_ && latency_is_adaptive_ && has_metrics_) {
        update_target_latency_(link_metrics_.peak_jitter, link_metrics_.mean_jitter,
                               latency_metrics_.fec_block_duration);
    }

    packet::stream_timestamp_diff_t actual_latency = 0;
    if (!measure_actual_latency_(actual_latency)) {
        return true;
    }

    if (enable_tolerance_checks_) {
        if (!check_actual_latency_(actual_latency)) {
            return false;
        }
    }

    if (enable_latency_adjustment_) {
        compute_scaling_(actual_latency);
    }

    if (dumper_) {
        dump_();
    }

    return true;
}

void LatencyTuner::advance_stream(packet::stream_timestamp_t duration) {
    roc_panic_if(init_status_ != status::StatusOK);

    stream_pos_ += duration;

    periodic_report_();
}

float LatencyTuner::fetch_scaling() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!has_new_freq_coeff_) {
        return 0;
    }

    has_new_freq_coeff_ = false;
    return freq_coeff_;
}

bool LatencyTuner::measure_actual_latency_(packet::stream_timestamp_diff_t& result) {
    switch (backend_) {
    case LatencyTunerBackend_Niq:
        if (!has_niq_latency_) {
            return false;
        }
        result = niq_latency_;
        return true;

    case LatencyTunerBackend_E2e:
        if (!has_e2e_latency_) {
            return false;
        }
        result = e2e_latency_;
        return true;

    case LatencyTunerBackend_Auto:
        break;
    }

    roc_panic("latency tuner: invalid backend: id=%d", (int)backend_);
}

bool LatencyTuner::check_actual_latency_(
    const packet::stream_timestamp_diff_t actual_latency) {
    // Queue is considered "stalling" if there were no new packets for
    // some period of time.
    const bool is_stalling = backend_ == LatencyTunerBackend_Niq
        && niq_stalling_ > max_stalling_ && max_stalling_ > 0;

    if (actual_latency < min_actual_latency_ && is_stalling) {
        // There are two possible reasons why queue latency becomes lower than minimum:
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
        // during notable amount of time, we assume that the second case takes place.
        return true;
    }

    if (actual_latency < min_actual_latency_ || actual_latency > max_actual_latency_) {
        roc_log(LogDebug,
                "latency tuner: latency out of bounds:"
                " latency=%ld(%.3fms) target=%ld(%.3fms)"
                " min=%ld(%.3fms) max=%ld(%.3fms) stale=%ld(%.3fms)",
                (long)actual_latency,
                sample_spec_.stream_timestamp_delta_2_ms(actual_latency),
                (long)cur_target_latency_,
                sample_spec_.stream_timestamp_delta_2_ms(cur_target_latency_),
                (long)min_target_latency_,
                sample_spec_.stream_timestamp_delta_2_ms(min_target_latency_),
                (long)max_target_latency_,
                sample_spec_.stream_timestamp_delta_2_ms(max_target_latency_),
                (long)niq_stalling_,
                sample_spec_.stream_timestamp_delta_2_ms(niq_stalling_));
        return false;
    }

    return true;
}

void LatencyTuner::compute_scaling_(packet::stream_timestamp_diff_t actual_latency) {
    if (actual_latency < 0) {
        actual_latency = 0;
    }

    if (packet::stream_timestamp_lt(stream_pos_, scale_pos_)) {
        return;
    }

    while (packet::stream_timestamp_ge(stream_pos_, scale_pos_)) {
        fe_->update_stream_position(stream_pos_);
        fe_->update_current_latency((packet::stream_timestamp_t)actual_latency);
        scale_pos_ += scale_interval_;
    }

    has_new_freq_coeff_ = true;

    freq_coeff_ = fe_->freq_coeff();
    freq_coeff_ = std::min(freq_coeff_, 1.0f + freq_coeff_max_delta_);
    freq_coeff_ = std::max(freq_coeff_, 1.0f - freq_coeff_max_delta_);
}

// Decides if the latency should be adjusted and orders fe_ to do so if needed.
//
// 1. Decides to decrease latency if current value is greater than upper threshold,
//    The target latency is supposed to change smoothly, so we just cut the current
//    latency value by some percentage.
//
// 2. Decides to increase latency if it is lesser than lower threshold (which
//    could be close or equal to target latency itself).
//    This could/should be done effectively as it could possibly mean that the user
//    is already perceives some losses.
//
//    NB: After the increasement the new latency target value must not be greater than
//        upper threshold in any circumstances.
//
void LatencyTuner::update_target_latency_(const core::nanoseconds_t peak_jitter_ns,
                                          const core::nanoseconds_t mean_jitter_ns,
                                          const core::nanoseconds_t fec_block_ns) {
    // If there is no active timeout, check if evaluated target latency is
    // significantly smaller than the latency in action so that we could decrease it.
    if (target_latency_state_ == TL_IDLE) {
        // Here we estimate what would be the perfect latency for this moment based on
        // jitter statistics. Later we'll use this value only for decision making if it
        // worth changing or we rather keep the current latency target untouched.
        const core::nanoseconds_t estimate = std::max(
            std::max(core::nanoseconds_t(peak_jitter_ns * max_jitter_overhead_),
                     core::nanoseconds_t(mean_jitter_ns * mean_jitter_overhead_)),
            fec_block_ns);

        const core::nanoseconds_t cur_tl_ns =
            sample_spec_.stream_timestamp_delta_2_ns(cur_target_latency_);

        if (estimate < cur_tl_ns && estimate * lat_update_upper_thrsh_ < cur_tl_ns
            && fe_->is_stable()) {
            try_decrease_latency_(estimate, cur_tl_ns);
        } else if (estimate > cur_tl_ns) {
            // If evaluated target latency is greater, than we must increase it.
            try_increase_latency_(estimate, cur_tl_ns);
        }
    } else if (target_latency_state_ == TL_COOLDOWN_AFTER_DEC
               && packet::stream_timestamp_diff(stream_pos_, last_target_latency_update_)
                   > cooldown_dec_timeout_) {
        // Waiting the timeout since the last decreasement.
        target_latency_state_ = TL_IDLE;
    } else if (target_latency_state_ == TL_STARTING
               && packet::stream_timestamp_diff(stream_pos_, last_target_latency_update_)
                   > starting_timeout_) {
        // Waiting the timeout since the startup.
        target_latency_state_ = TL_IDLE;
    } else if (target_latency_state_ == TL_COOLDOWN_AFTER_INC
               && packet::stream_timestamp_diff(stream_pos_, last_target_latency_update_)
                   > cooldown_inc_timeout_) {
        // Waiting the timeout since the last increasement.
        target_latency_state_ = TL_IDLE;
    }
}

void LatencyTuner::try_increase_latency_(const core::nanoseconds_t estimate,
                                         const core::nanoseconds_t cur_tl_ns) {
    core::nanoseconds_t new_tl_ns =
        (core::nanoseconds_t)(estimate * lat_update_inc_step_);

    packet::stream_timestamp_diff_t new_tl_ts =
        sample_spec_.ns_2_stream_timestamp_delta(new_tl_ns);

    if (new_tl_ts >= max_target_latency_ && cur_target_latency_ == max_target_latency_) {
        if (last_lat_limiter_.allow()) {
            roc_log(LogDebug,
                    "latency tuner:"
                    " not increasing target latency higher than limit %ld(%.3fms)",
                    (long)max_target_latency_,
                    sample_spec_.stream_timestamp_delta_2_ms(max_target_latency_));
        }
        return;
    }

    if (new_tl_ts > max_target_latency_) {
        if (last_lat_limiter_.allow()) {
            roc_log(LogDebug,
                    "latency tuner:"
                    " capping target latency %ld(%.3fms)"
                    " as max limit is lower %ld(%.3fms)",
                    (long)new_tl_ts, (double)new_tl_ns / core::Millisecond,
                    (long)max_target_latency_,
                    sample_spec_.stream_timestamp_delta_2_ms(max_target_latency_));

            if (sample_spec_.ns_2_stream_timestamp_delta(
                    latency_metrics_.fec_block_duration)
                >= max_target_latency_) {
                roc_log(LogNote,
                        "latency tuner: fec block is longer than max latency:"
                        " fec_blk=%.3fms max_latency=%.3fms",
                        (double)latency_metrics_.fec_block_duration / core::Millisecond,
                        (double)max_target_latency_ / core::Millisecond);
            }
        }
        new_tl_ns = sample_spec_.stream_timestamp_2_ns(
            (packet::stream_timestamp_t)max_target_latency_);
        new_tl_ts = max_target_latency_;
    }

    roc_log(LogNote,
            "latency tuner:"
            " increasing target latency %ld(%.3fms) => %ld(%.3fms)",
            (long)cur_target_latency_, (double)cur_tl_ns / core::Millisecond,
            (long)new_tl_ts, (double)new_tl_ns / core::Millisecond);

    roc_log(LogDebug,
            "latency tuner:"
            " estimate %.3fms * %.3f = %.3fms,"
            " new_tl %.3fms * %f = %.3fms",
            (double)estimate / core::Millisecond, (double)lat_update_upper_thrsh_,
            (double)estimate * (double)lat_update_upper_thrsh_ / core::Millisecond,
            (double)cur_tl_ns / core::Millisecond, (double)lat_update_inc_step_,
            (double)new_tl_ns / core::Millisecond);

    cur_target_latency_ = new_tl_ts;
    last_target_latency_update_ = stream_pos_;
    target_latency_state_ = TL_COOLDOWN_AFTER_INC;

    fe_->update_target_latency((packet::stream_timestamp_t)cur_target_latency_);
}

void LatencyTuner::try_decrease_latency_(const core::nanoseconds_t estimate,
                                         const core::nanoseconds_t cur_tl_ns) {
    core::nanoseconds_t new_tl_ns =
        (core::nanoseconds_t)(cur_tl_ns * lat_update_dec_step_);

    packet::stream_timestamp_diff_t new_tl_ts =
        sample_spec_.ns_2_stream_timestamp_delta(new_tl_ns);

    if (new_tl_ts <= min_target_latency_ && cur_target_latency_ == min_target_latency_) {
        if (last_lat_limiter_.allow()) {
            roc_log(LogDebug,
                    "latency tuner:"
                    " not decreasing target latency lower than limit %ld(%.3fms)",
                    (long)min_target_latency_,
                    sample_spec_.stream_timestamp_delta_2_ms(min_target_latency_));
        }
        return;
    }

    if (new_tl_ts < min_target_latency_) {
        if (last_lat_limiter_.allow()) {
            roc_log(LogDebug,
                    "latency tuner:"
                    " capping target latency %ld(%.3fms)"
                    " as min limit is higher %ld(%.3fms)",
                    (long)new_tl_ts, (double)new_tl_ns / core::Millisecond,
                    (long)min_target_latency_,
                    sample_spec_.stream_timestamp_delta_2_ms(min_target_latency_));
        }
        new_tl_ns = sample_spec_.stream_timestamp_2_ns(
            (packet::stream_timestamp_t)min_target_latency_);
        new_tl_ts = min_target_latency_;
    }

    roc_log(LogNote,
            "latency tuner:"
            " decreasing target latency %ld(%.3fms) => %ld(%.3fms)",
            (long)cur_target_latency_, (double)cur_tl_ns / core::Millisecond,
            (long)new_tl_ts, (double)new_tl_ns / core::Millisecond);

    roc_log(LogDebug,
            "latency tuner:"
            " estimate %.3fms * %.3f = %.3fms,"
            " new_tl %.3fms * %f = %.3fms",
            (double)estimate / core::Millisecond, (double)lat_update_upper_thrsh_,
            (double)estimate * (double)lat_update_upper_thrsh_ / core::Millisecond,
            (double)cur_tl_ns / core::Millisecond, (double)lat_update_dec_step_,
            (double)new_tl_ns / core::Millisecond);

    cur_target_latency_ = new_tl_ts;
    last_target_latency_update_ = stream_pos_;
    target_latency_state_ = TL_COOLDOWN_AFTER_DEC;

    fe_->update_target_latency((packet::stream_timestamp_t)cur_target_latency_);
}

void LatencyTuner::periodic_report_() {
    if (packet::stream_timestamp_lt(stream_pos_, report_pos_)) {
        return;
    }

    while (packet::stream_timestamp_ge(stream_pos_, report_pos_)) {
        report_pos_ += report_interval_;
    }

    roc_log(LogDebug,
            "latency tuner:"
            " e2e_latency=%ld(%.3fms) niq_latency=%ld(%.3fms) target_latency=%ld(%.3fms)"
            " stale=%ld(%.3fms)"
            " packets(lost/exp)=%ld/%ld jitter(mean/peak)=%.3fms/%.3fms"
            " fec_blk=%.3fms"
            " fe=%.6f eff_fe=%.6f fe_stbl=%s",
            // e2e_latency, niq_latency, target_latency
            (long)e2e_latency_, sample_spec_.stream_timestamp_delta_2_ms(e2e_latency_),
            (long)niq_latency_, sample_spec_.stream_timestamp_delta_2_ms(niq_latency_),
            (long)cur_target_latency_,
            sample_spec_.stream_timestamp_delta_2_ms(cur_target_latency_),
            // stale
            (long)niq_stalling_, sample_spec_.stream_timestamp_delta_2_ms(niq_stalling_),
            // packets
            (long)link_metrics_.lost_packets, (long)link_metrics_.expected_packets,
            // jitter
            (double)link_metrics_.mean_jitter / core::Millisecond,
            (double)link_metrics_.peak_jitter / core::Millisecond,
            // fec_blk
            (double)latency_metrics_.fec_block_duration / core::Millisecond,
            // fe, eff_fe, fe_stbl
            (double)(fe_ ? fe_->freq_coeff() : 0), (double)freq_coeff_,
            fe_ && fe_->is_stable() ? "true" : "false");
}

void LatencyTuner::dump_() {
    dbgio::CsvEntry e;
    e.type = 't';
    e.n_fields = 3;
    e.fields[0] = core::timestamp(core::ClockUnix);
    e.fields[1] = niq_latency_;
    e.fields[2] = cur_target_latency_;
    dumper_->write(e);
}

} // namespace audio
} // namespace roc
