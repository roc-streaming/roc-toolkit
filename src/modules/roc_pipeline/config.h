/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/config.h
//! @brief Pipeline config.

#ifndef ROC_PIPELINE_CONFIG_H_
#define ROC_PIPELINE_CONFIG_H_

#include "roc_audio/latency_monitor.h"
#include "roc_audio/resampler_config.h"
#include "roc_audio/watchdog.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_fec/codec_config.h"
#include "roc_fec/reader.h"
#include "roc_fec/writer.h"
#include "roc_packet/units.h"
#include "roc_pipeline/port.h"
#include "roc_rtp/headers.h"
#include "roc_rtp/validator.h"

namespace roc {
namespace pipeline {

//! Default sample rate, number of samples per second.
const size_t DefaultSampleRate = 44100;

//! Default channel mask.
const packet::channel_mask_t DefaultChannelMask = 0x3;

//! Default packet length.
const core::nanoseconds_t DefaultPacketLength = 7 * core::Millisecond;

//! Default latency.
const core::nanoseconds_t DefaultLatency = 200 * core::Millisecond;

//! Default internal frame size.
const size_t DefaultInternalFrameSize = 640;

//! Default minum latency relative to target latency.
const int DefaultMinLatencyFactor = -1;

//! Default maximum latency relative to target latency.
const int DefaultMaxLatencyFactor = 2;

//! Port parameters.
//! @remarks
//!  On receiver, defines a listened port parameters. On sender,
//!  defines a destination port parameters.
struct PortConfig {
    //! Port address.
    address::SocketAddr address;

    //! Port protocol.
    PortProtocol protocol;

    PortConfig()
        : protocol(Proto_None) {
    }
};

//! Sender parameters.
struct SenderConfig {
    //! Resampler parameters.
    audio::ResamplerConfig resampler;

    //! To specify which resampling backend will be used.
    audio::ResamplerBackend resampler_backend;

    //! FEC writer parameters.
    fec::WriterConfig fec_writer;

    //! FEC encoder parameters.
    fec::CodecConfig fec_encoder;

    //! Number of samples per second per channel.
    size_t input_sample_rate;

    //! Channel mask.
    packet::channel_mask_t input_channels;

    //! Number of samples for internal frames.
    size_t internal_frame_size;

    //! Packet length, in nanoseconds.
    core::nanoseconds_t packet_length;

    //! RTP payload type for audio packets.
    rtp::PayloadType payload_type;

    //! Resample frames with a constant ratio.
    bool resampling;

    //! Interleave packets.
    bool interleaving;

    //! Constrain receiver speed using a CPU timer according to the sample rate.
    bool timing;

    //! Fill unitialized data with large values to make them more noticable.
    bool poisoning;

    SenderConfig()
        : resampler_backend(audio::ResamplerBackend_Builtin)
        , input_sample_rate(DefaultSampleRate)
        , input_channels(DefaultChannelMask)
        , internal_frame_size(DefaultInternalFrameSize)
        , packet_length(DefaultPacketLength)
        , payload_type(rtp::PayloadType_L16_Stereo)
        , resampling(false)
        , interleaving(false)
        , timing(false)
        , poisoning(false) {
    }
};

//! Receiver session parameters.
//! @remarks
//!  Defines per-session receiver parameters.
struct ReceiverSessionConfig {
    //! Target latency, nanoseconds.
    core::nanoseconds_t target_latency;

    //! Channel mask.
    packet::channel_mask_t channels;

    //! Packet payload type.
    unsigned int payload_type;

    //! FEC reader parameters.
    fec::ReaderConfig fec_reader;

    //! FEC decoder parameters.
    fec::CodecConfig fec_decoder;

    //! RTP validator parameters.
    rtp::ValidatorConfig rtp_validator;

    //! LatencyMonitor parameters.
    audio::LatencyMonitorConfig latency_monitor;

    //! Watchdog parameters.
    audio::WatchdogConfig watchdog;

    //! Resampler parameters.
    audio::ResamplerConfig resampler;

    //! To specify which resampling backend will be used.
    audio::ResamplerBackend resampler_backend;

    ReceiverSessionConfig()
        : target_latency(DefaultLatency)
        , channels(DefaultChannelMask)
        , payload_type(0)
        , resampler_backend(audio::ResamplerBackend_Builtin) {
        latency_monitor.min_latency = target_latency * DefaultMinLatencyFactor;
        latency_monitor.max_latency = target_latency * DefaultMaxLatencyFactor;
    }
};

//! Receiver common parameters.
//! @remarks
//!  Defines receiver parameters common for all sessions.
struct ReceiverCommonConfig {
    //! Number of samples per second per channel.
    size_t output_sample_rate;

    //! Channel mask.
    packet::channel_mask_t output_channels;

    //! Number of samples for internal frames.
    size_t internal_frame_size;

    //! Perform resampling to compensate sender and receiver frequency difference.
    bool resampling;

    //! Constrain receiver speed using a CPU timer according to the sample rate.
    bool timing;

    //! Fill uninitialized data with large values to make them more noticeable.
    bool poisoning;

    //! Insert weird beeps instead of silence on packet loss.
    bool beeping;

    ReceiverCommonConfig()
        : output_sample_rate(DefaultSampleRate)
        , output_channels(DefaultChannelMask)
        , internal_frame_size(DefaultInternalFrameSize)
        , resampling(false)
        , timing(false)
        , poisoning(false)
        , beeping(false) {
    }
};

//! Receiver parameters.
struct ReceiverConfig {
    //! Default parameters for receiver session.
    ReceiverSessionConfig default_session;

    //! Parameters common for all sessions.
    ReceiverCommonConfig common;
};

//! Converter parameters.
struct ConverterConfig {
    //! Resampler parameters.
    audio::ResamplerConfig resampler;

    //! To specify which resampling backend will be used.
    audio::ResamplerBackend resampler_backend;

    //! Number of samples per second per channel.
    size_t input_sample_rate;

    //! Number of samples per second per channel.
    size_t output_sample_rate;

    //! Input channel mask.
    packet::channel_mask_t input_channels;

    //! Output channel mask.
    packet::channel_mask_t output_channels;

    //! Number of samples for internal frames.
    size_t internal_frame_size;

    //! Resample frames with a constant ratio.
    bool resampling;

    //! Fill unitialized data with large values to make them more noticable.
    bool poisoning;

    ConverterConfig()
        : resampler_backend(audio::ResamplerBackend_Builtin)
        , input_sample_rate(DefaultSampleRate)
        , output_sample_rate(DefaultSampleRate)
        , input_channels(DefaultChannelMask)
        , output_channels(DefaultChannelMask)
        , internal_frame_size(DefaultInternalFrameSize)
        , resampling(false)
        , poisoning(false) {
    }
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CONFIG_H_
