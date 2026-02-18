/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/config.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace pipeline {

// SenderSinkConfig

SenderSinkConfig::SenderSinkConfig()
    : input_sample_spec(DefaultSampleSpec)
    , payload_type(rtp::PayloadType_L16_Stereo)
    , packet_length(DefaultPacketLength)
    , enable_cpu_clock(false)
    , enable_auto_cts(false)
    , enable_interleaving(false)
    , enable_profiling(false) {
}

bool SenderSinkConfig::deduce_defaults(audio::ProcessorMap& processor_map) {
    if (!latency.deduce_defaults(DefaultLatency, false)) {
        return false;
    }

    if (!freq_est.deduce_defaults(latency.tuner_profile)) {
        return false;
    }

    if (!resampler.deduce_defaults(processor_map, latency.tuner_backend,
                                   latency.tuner_profile)) {
        return false;
    }

    return true;
}

// SenderSlotConfig

SenderSlotConfig::SenderSlotConfig() {
}

bool SenderSlotConfig::deduce_defaults() {
    return true;
}

// ReceiverCommonConfig

ReceiverCommonConfig::ReceiverCommonConfig()
    : output_sample_spec(DefaultSampleSpec)
    , enable_cpu_clock(false)
    , enable_auto_reclock(false)
    , enable_profiling(false) {
}

bool ReceiverCommonConfig::deduce_defaults(audio::ProcessorMap& processor_map) {
    return true;
}

// ReceiverSessionConfig

ReceiverSessionConfig::ReceiverSessionConfig()
    : payload_type(0) {
}

bool ReceiverSessionConfig::deduce_defaults(audio::ProcessorMap& processor_map) {
    if (!plc.deduce_defaults()) {
        return false;
    }

    if (!latency.deduce_defaults(DefaultLatency, true)) {
        return false;
    }

    if (!jitter_meter.deduce_defaults(latency.tuner_profile)) {
        return false;
    }

    if (!freq_est.deduce_defaults(latency.tuner_profile)) {
        return false;
    }

    if (!resampler.deduce_defaults(processor_map, latency.tuner_backend,
                                   latency.tuner_profile)) {
        return false;
    }

    if (!watchdog.deduce_defaults(DefaultLatency, latency.target_latency)) {
        return false;
    }

    return true;
}

// ReceiverSourceConfig

ReceiverSourceConfig::ReceiverSourceConfig() {
}

bool ReceiverSourceConfig::deduce_defaults(audio::ProcessorMap& processor_map) {
    if (!common.deduce_defaults(processor_map)) {
        return false;
    }

    if (!session_defaults.deduce_defaults(processor_map)) {
        return false;
    }

    return true;
}

// ReceiverSlotConfig

ReceiverSlotConfig::ReceiverSlotConfig()
    : enable_routing(true) {
}

bool ReceiverSlotConfig::deduce_defaults() {
    return true;
}

// TranscoderConfig

TranscoderConfig::TranscoderConfig()
    : input_sample_spec(DefaultSampleSpec)
    , output_sample_spec(DefaultSampleSpec)
    , enable_profiling(false) {
}

bool TranscoderConfig::deduce_defaults(audio::ProcessorMap& processor_map) {
    if (!resampler.deduce_defaults(processor_map, audio::LatencyTunerBackend_Auto,
                                   audio::LatencyTunerProfile_Auto)) {
        return false;
    }

    return true;
}

} // namespace pipeline
} // namespace roc
