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

#include "roc_address/protocol.h"
#include "roc_audio/latency_monitor.h"
#include "roc_audio/profiler.h"
#include "roc_audio/resampler_backend.h"
#include "roc_audio/resampler_profile.h"
#include "roc_audio/watchdog.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_fec/codec_config.h"
#include "roc_fec/reader.h"
#include "roc_fec/writer.h"
#include "roc_packet/units.h"
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

//! Default internal frame length.
const core::nanoseconds_t DefaultInternalFrameLength = 7 * core::Millisecond;

//! Default minum latency relative to target latency.
const int DefaultMinLatencyFactor = -1;

//! Default maximum latency relative to target latency.
const int DefaultMaxLatencyFactor = 2;

//! Task processing parameters.
struct TaskConfig {
    //! Enable precise task scheduling mode (default).
    //! The other settings have effect only when this is set to true.
    //! When enabled, pipeline processes tasks in dedicated time intervals between
    //! sub-frame and between frames, trying to prevent time collisions between
    //! task and frame processing.
    bool enable_precise_task_scheduling;

    //! Minimum frame duration between processing tasks.
    //! In-frame task processing does not happen until at least given number
    //! of samples is processed.
    //! Set to zero to allow task processing between frames of any size.
    core::nanoseconds_t min_frame_length_between_tasks;

    //! Maximum frame duration between processing tasks.
    //! If the frame is larger than this size, it is split into multiple subframes
    //! to allow task processing between the sub-frames.
    //! Set to zero to disable frame splitting.
    core::nanoseconds_t max_frame_length_between_tasks;

    //! Mximum task processing duration happening immediatelly after processing a frame.
    //! If this period expires and there are still pending tasks, asynchronous
    //! task processing is scheduled.
    //! At least one task is always processed after each frame, even if this
    //! setting is too small.
    core::nanoseconds_t max_inframe_task_processing;

    //! Time interval during which no task processing is allowed.
    //! This setting is used to prohibit task processing during the time when
    //! next read() or write() call is expected.
    //! Since it can not be calculated abolutely precisely, and there is always
    //! thread switch overhead, scheduler jitter clock drift, we use a wide interval.
    core::nanoseconds_t task_processing_prohibited_interval;

    TaskConfig()
        : enable_precise_task_scheduling(true)
        , min_frame_length_between_tasks(200 * core::Microsecond)
        , max_frame_length_between_tasks(DefaultInternalFrameLength)
        , max_inframe_task_processing(20 * core::Microsecond)
        , task_processing_prohibited_interval(200 * core::Microsecond) {
    }
};

//! Sender parameters.
struct SenderConfig {
    //! Task processing parameters.
    TaskConfig tasks;

    //! To specify which resampling backend will be used.
    audio::ResamplerBackend resampler_backend;

    //! Resampler profile.
    audio::ResamplerProfile resampler_profile;

    //! FEC writer parameters.
    fec::WriterConfig fec_writer;

    //! FEC encoder parameters.
    fec::CodecConfig fec_encoder;

    //! Number of samples per second per channel.
    size_t input_sample_rate;

    //! Channel mask.
    packet::channel_mask_t input_channels;

    //! Duration of the internal frames, in nanoseconds.
    core::nanoseconds_t internal_frame_length;

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

    //! Profile moving average of frames being written.
    bool profiling;

    //! Profiler configuration.
    audio::ProfilerConfig profiler_config;

    SenderConfig()
        : resampler_backend(audio::ResamplerBackend_Builtin)
        , resampler_profile(audio::ResamplerProfile_Medium)
        , input_sample_rate(DefaultSampleRate)
        , input_channels(DefaultChannelMask)
        , internal_frame_length(DefaultInternalFrameLength)
        , packet_length(DefaultPacketLength)
        , payload_type(rtp::PayloadType_L16_Stereo)
        , resampling(false)
        , interleaving(false)
        , timing(false)
        , poisoning(false)
        , profiling(false) {
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

    //! To specify which resampling backend will be used.
    audio::ResamplerBackend resampler_backend;

    //! Resampler profile.
    audio::ResamplerProfile resampler_profile;

    ReceiverSessionConfig()
        : target_latency(DefaultLatency)
        , channels(DefaultChannelMask)
        , payload_type(0)
        , resampler_backend(audio::ResamplerBackend_Builtin)
        , resampler_profile(audio::ResamplerProfile_Medium) {
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

    //! Duration of the internal frames, in nanoseconds.
    core::nanoseconds_t internal_frame_length;

    //! Perform resampling to compensate sender and receiver frequency difference.
    bool resampling;

    //! Constrain receiver speed using a CPU timer according to the sample rate.
    bool timing;

    //! Fill uninitialized data with large values to make them more noticeable.
    bool poisoning;

    //! Profile moving average of frames being written.
    bool profiling;

    //! Profiler configuration.
    audio::ProfilerConfig profiler_config;

    //! Insert weird beeps instead of silence on packet loss.
    bool beeping;

    ReceiverCommonConfig()
        : output_sample_rate(DefaultSampleRate)
        , output_channels(DefaultChannelMask)
        , internal_frame_length(DefaultInternalFrameLength)
        , resampling(false)
        , timing(false)
        , poisoning(false)
        , profiling(false)
        , beeping(false) {
    }
};

//! Receiver parameters.
struct ReceiverConfig {
    //! Default parameters for receiver session.
    ReceiverSessionConfig default_session;

    //! Parameters common for all sessions.
    ReceiverCommonConfig common;

    //! Task processing parameters.
    TaskConfig tasks;
};

//! Converter parameters.
struct ConverterConfig {
    //! To specify which resampling backend will be used.
    audio::ResamplerBackend resampler_backend;

    //! Resampler profile.
    audio::ResamplerProfile resampler_profile;

    //! Number of samples per second per channel.
    size_t input_sample_rate;

    //! Number of samples per second per channel.
    size_t output_sample_rate;

    //! Input channel mask.
    packet::channel_mask_t input_channels;

    //! Output channel mask.
    packet::channel_mask_t output_channels;

    //! Duration of the internal frames, in nanoseconds.
    core::nanoseconds_t internal_frame_length;

    //! Resample frames with a constant ratio.
    bool resampling;

    //! Fill unitialized data with large values to make them more noticable.
    bool poisoning;

    //! Profile moving average of frames being written.
    bool profiling;

    //! Profiler configuration.
    audio::ProfilerConfig profiler_config;

    ConverterConfig()
        : resampler_backend(audio::ResamplerBackend_Builtin)
        , resampler_profile(audio::ResamplerProfile_Medium)
        , input_sample_rate(DefaultSampleRate)
        , output_sample_rate(DefaultSampleRate)
        , input_channels(DefaultChannelMask)
        , output_channels(DefaultChannelMask)
        , internal_frame_length(DefaultInternalFrameLength)
        , resampling(false)
        , poisoning(false)
        , profiling(false) {
    }
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CONFIG_H_
