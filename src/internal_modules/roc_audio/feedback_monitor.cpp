/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/feedback_monitor.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

FeedbackMonitor::FeedbackMonitor(IFrameWriter& writer,
                                 ResamplerWriter* resampler,
                                 const FeedbackConfig& feedback_config,
                                 const LatencyConfig& latency_config,
                                 const SampleSpec& sample_spec)
    : tuner_(latency_config, sample_spec)
    , latency_metrics_()
    , has_feedback_(false)
    , last_feedback_ts_(0)
    , feedback_timeout_(feedback_config.source_timeout)
    , writer_(writer)
    , resampler_(resampler)
    , enable_scaling_(latency_config.tuner_profile != audio::LatencyTunerProfile_Intact)
    , source_(0)
    , source_change_limiter_(feedback_config.source_cooldown)
    , sample_spec_(sample_spec)
    , started_(false)
    , valid_(false) {
    if (!tuner_.is_valid()) {
        return;
    }

    if (enable_scaling_) {
        if (!init_scaling_()) {
            return;
        }
    }

    valid_ = true;
}

bool FeedbackMonitor::is_valid() const {
    return valid_;
}

bool FeedbackMonitor::is_started() const {
    return started_;
}

void FeedbackMonitor::start() {
    roc_panic_if(!is_valid());

    if (started_) {
        return;
    }

    roc_log(LogDebug, "feedback monitor: start gathering feedback");
    started_ = true;
}

void FeedbackMonitor::process_feedback(packet::stream_source_t source_id,
                                       const LatencyMetrics& latency_metrics,
                                       const packet::LinkMetrics& link_metrics) {
    roc_panic_if(!is_valid());

    if (!started_) {
        return;
    }

    if (!has_feedback_) {
        roc_log(LogInfo, "feedback monitor: got first report from receiver: source=%lu",
                (unsigned long)source_id);
        source_ = source_id;
    }

    if (has_feedback_ && source_ != source_id) {
        if (!source_change_limiter_.allow()) {
            // Protection from inadequately frequent SSRC changes.
            // Can happen is feedback monitor is mistakenly created when multiple
            // receivers exists for a single sender, which is not supported.
            // This also protects from outdated reports delivered from recently
            // restarted receiver.
            return;
        }

        roc_log(LogInfo,
                "feedback monitor: detected source change:"
                " old_source=%lu new_source=%lu",
                (unsigned long)source_, (unsigned long)source_id);

        source_ = source_id;
    }

    latency_metrics_ = latency_metrics;
    link_metrics_ = link_metrics;

    has_feedback_ = true;
    last_feedback_ts_ = core::timestamp(core::ClockMonotonic);
}

void FeedbackMonitor::write(Frame& frame) {
    roc_panic_if(!is_valid());

    if (started_) {
        if (!update_tuner_(frame.duration())) {
            // TODO(gh-674): change sender SSRC to restart session
        }

        if (enable_scaling_) {
            if (!update_scaling_()) {
                // TODO(gh-183): return status code
                roc_panic("feedback monitor: update failed");
            }
        }
    }

    writer_.write(frame);
}

size_t FeedbackMonitor::num_participants() const {
    // TODO(gh-674): collect per-session metrics
    return has_feedback_ ? 1 : 0;
}

const LatencyMetrics& FeedbackMonitor::latency_metrics(size_t part_index) const {
    roc_panic_if_msg(part_index >= num_participants(),
                     "feedback monitor: participant index out of bounds:"
                     " index=%lu max=%lu",
                     (unsigned long)part_index, (unsigned long)num_participants());

    // TODO(gh-674): collect per-session metrics
    return latency_metrics_;
}

const packet::LinkMetrics& FeedbackMonitor::link_metrics(size_t part_index) const {
    roc_panic_if_msg(part_index >= num_participants(),
                     "feedback monitor: participant index out of bounds:"
                     " index=%lu max=%lu",
                     (unsigned long)part_index, (unsigned long)num_participants());

    // TODO(gh-674): collect per-session metrics
    return link_metrics_;
}

bool FeedbackMonitor::update_tuner_(packet::stream_timestamp_t duration) {
    if (!has_feedback_) {
        return true;
    }

    if (core::timestamp(core::ClockMonotonic) - last_feedback_ts_ > feedback_timeout_) {
        roc_log(LogInfo,
                "feedback monitor: no reports from receiver during timeout:"
                " source=%lu timeout=%.3fms",
                (unsigned long)source_, (double)feedback_timeout_ / core::Millisecond);

        has_feedback_ = false;
        last_feedback_ts_ = 0;
        source_ = 0;

        return true;
    }

    tuner_.write_metrics(latency_metrics_, link_metrics_);

    if (!tuner_.update_stream()) {
        return false;
    }

    tuner_.advance_stream(duration);

    return true;
}

bool FeedbackMonitor::init_scaling_() {
    roc_panic_if_not(resampler_);

    if (!resampler_->set_scaling(1.0f)) {
        roc_log(LogError, "feedback monitor: can't set initial scaling");
        return false;
    }

    return true;
}

bool FeedbackMonitor::update_scaling_() {
    roc_panic_if_not(resampler_);

    const float scaling = tuner_.get_scaling();

    if (scaling > 0) {
        if (!resampler_->set_scaling(scaling)) {
            roc_log(LogDebug,
                    "feedback monitor: scaling factor out of bounds: scaling=%.6f",
                    (double)scaling);
            return false;
        }
    }

    return true;
}

} // namespace audio
} // namespace roc
