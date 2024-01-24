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

SenderConfig::SenderConfig()
    : input_sample_spec(DefaultSampleSpec)
    , packet_length(DefaultPacketLength)
    , payload_type(rtp::PayloadType_L16_Stereo)
    , enable_interleaving(false)
    , enable_timing(false)
    , enable_auto_cts(false)
    , enable_profiling(false) {
}

void SenderConfig::deduce_defaults() {
    resampler.deduce_defaults(audio::FreqEstimatorInput_Disable,
                              audio::FreqEstimatorProfile_Default);
}

ReceiverSessionConfig::ReceiverSessionConfig()
    : payload_type(0)
    , latency(DefaultLatency) {
}

void ReceiverSessionConfig::deduce_defaults() {
    latency.deduce_defaults(true);
    watchdog.deduce_defaults(latency.target_latency);
    resampler.deduce_defaults(latency.fe_input, latency.fe_profile);
}

ReceiverCommonConfig::ReceiverCommonConfig()
    : output_sample_spec(DefaultSampleSpec)
    , enable_timing(false)
    , enable_auto_reclock(false)
    , enable_profiling(false)
    , enable_beeping(false) {
}

void ReceiverCommonConfig::deduce_defaults() {
}

ReceiverConfig::ReceiverConfig() {
}

void ReceiverConfig::deduce_defaults() {
    default_session.deduce_defaults();
    common.deduce_defaults();
}

TranscoderConfig::TranscoderConfig()
    : input_sample_spec(DefaultSampleSpec)
    , output_sample_spec(DefaultSampleSpec)
    , enable_profiling(false) {
}

void TranscoderConfig::deduce_defaults() {
    resampler.deduce_defaults(audio::FreqEstimatorInput_Disable,
                              audio::FreqEstimatorProfile_Default);
}

} // namespace pipeline
} // namespace roc
