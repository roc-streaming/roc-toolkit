/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/config.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/log.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace pipeline {

SenderSinkConfig::SenderSinkConfig()
    : input_sample_spec(DefaultSampleSpec)
    , payload_type(rtp::PayloadType_L16_Stereo)
    , packet_length(0)
    , packet_mtu(0)
    , enable_timing(false)
    , enable_auto_duration(false)
    , enable_auto_cts(false)
    , enable_profiling(false)
    , enable_interleaving(false) {
}

void SenderSinkConfig::deduce_defaults(const audio::SampleSpec& encoding_sample_spec) {
    // Resolve packet_length from packet_mtu if needed.
    if (packet_length == 0 && packet_mtu == 0) {
        // Neither set: derive packet_length from DefaultPacketMtu, then use
        // min(DefaultPacketLength, mtu_derived_length).
        if (encoding_sample_spec.is_valid() && encoding_sample_spec.pcm_format()
            != audio::PcmFormat_Invalid) {
            const size_t payload_bytes =
                DefaultPacketMtu > RtpHeaderSize ? DefaultPacketMtu - RtpHeaderSize : 0;
            const core::nanoseconds_t mtu_length =
                encoding_sample_spec.bytes_2_ns(payload_bytes);
            if (mtu_length > 0) {
                packet_length = std::min(DefaultPacketLength, mtu_length);
            } else {
                packet_length = DefaultPacketLength;
            }
        } else {
            packet_length = DefaultPacketLength;
        }
    } else if (packet_length == 0 && packet_mtu != 0) {
        // Only MTU set: derive packet_length from it.
        if (encoding_sample_spec.is_valid() && encoding_sample_spec.pcm_format()
            != audio::PcmFormat_Invalid) {
            const size_t payload_bytes =
                packet_mtu > RtpHeaderSize ? packet_mtu - RtpHeaderSize : 0;
            const core::nanoseconds_t mtu_length =
                encoding_sample_spec.bytes_2_ns(payload_bytes);
            packet_length = mtu_length > 0 ? mtu_length : DefaultPacketLength;
        } else {
            packet_length = DefaultPacketLength;
        }
    } else if (packet_length != 0 && packet_mtu != 0) {
        // Both set: warn and pick the one that leads to smaller packet size.
        if (encoding_sample_spec.is_valid() && encoding_sample_spec.pcm_format()
            != audio::PcmFormat_Invalid) {
            const size_t payload_bytes =
                packet_mtu > RtpHeaderSize ? packet_mtu - RtpHeaderSize : 0;
            const core::nanoseconds_t mtu_length =
                encoding_sample_spec.bytes_2_ns(payload_bytes);
            if (mtu_length > 0 && mtu_length < packet_length) {
                roc_log(LogInfo,
                        "sender config: both packet_length and packet_mtu are set;"
                        " using packet_mtu-derived length (%.3fms) since it is smaller"
                        " than packet_length (%.3fms)",
                        (double)mtu_length / core::Millisecond,
                        (double)packet_length / core::Millisecond);
                packet_length = mtu_length;
            } else {
                roc_log(LogInfo,
                        "sender config: both packet_length and packet_mtu are set;"
                        " using packet_length (%.3fms) since it is smaller"
                        " than packet_mtu-derived length (%.3fms)",
                        (double)packet_length / core::Millisecond,
                        (double)mtu_length / core::Millisecond);
            }
        }
        // else: can't compute mtu_length, keep packet_length as-is
    }
    // else: only packet_length set — use it as-is

    latency.deduce_defaults(DefaultLatency, false);
    resampler.deduce_defaults(latency.tuner_backend, latency.tuner_profile);
}

SenderSlotConfig::SenderSlotConfig() {
}

void SenderSlotConfig::deduce_defaults() {
}

ReceiverCommonConfig::ReceiverCommonConfig()
    : output_sample_spec(DefaultSampleSpec)
    , enable_timing(false)
    , enable_auto_reclock(false)
    , enable_profiling(false) {
}

void ReceiverCommonConfig::deduce_defaults() {
}

ReceiverSessionConfig::ReceiverSessionConfig()
    : payload_type(0)
    , enable_beeping(false) {
}

void ReceiverSessionConfig::deduce_defaults() {
    latency.deduce_defaults(DefaultLatency, true);
    watchdog.deduce_defaults(latency.target_latency);
    resampler.deduce_defaults(latency.tuner_backend, latency.tuner_profile);
}

ReceiverSourceConfig::ReceiverSourceConfig() {
}

void ReceiverSourceConfig::deduce_defaults() {
    common.deduce_defaults();
    session_defaults.deduce_defaults();
}

ReceiverSlotConfig::ReceiverSlotConfig()
    : enable_routing(true) {
}

void ReceiverSlotConfig::deduce_defaults() {
}

TranscoderConfig::TranscoderConfig()
    : input_sample_spec(DefaultSampleSpec)
    , output_sample_spec(DefaultSampleSpec)
    , enable_profiling(false) {
}

void TranscoderConfig::deduce_defaults() {
    resampler.deduce_defaults(audio::LatencyTunerBackend_Default,
                              audio::LatencyTunerProfile_Default);
}

} // namespace pipeline
} // namespace roc
