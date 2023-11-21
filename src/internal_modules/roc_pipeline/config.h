/*
 * Copyright (c) 2017 Roc Streaming authors
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
#include "roc_audio/freq_estimator.h"
#include "roc_audio/latency_monitor.h"
#include "roc_audio/profiler.h"
#include "roc_audio/resampler_backend.h"
#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_profile.h"
#include "roc_audio/sample_spec.h"
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

//! Default sample specification.
static const audio::SampleSpec DefaultSampleSpec(DefaultSampleRate,
                                                 audio::ChanLayout_Surround,
                                                 audio::ChanOrder_Smpte,
                                                 audio::ChanMask_Surround_Stereo);

//! Default packet length.
//! @remarks
//!  5ms works well on majority Wi-Fi networks and allows rather low latencies. However,
//!  a lower length may be required depending on network MTU, e.g. for Internet.
const core::nanoseconds_t DefaultPacketLength = 5 * core::Millisecond;

//! Default latency.
//! @remarks
//!  200ms works well on majority Wi-Fi networks and is not too annoying. However, many
//!  networks allow lower latencies, and some networks require higher.
const core::nanoseconds_t DefaultLatency = 200 * core::Millisecond;

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
        , max_frame_length_between_tasks(1 * core::Millisecond)
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

    //! Input sample spec
    audio::SampleSpec input_sample_spec;

    //! Packet length, in nanoseconds.
    core::nanoseconds_t packet_length;

    //! RTP payload type for audio packets.
    unsigned payload_type;

    //! Interleave packets.
    bool enable_interleaving;

    //! Constrain receiver speed using a CPU timer according to the sample rate.
    bool enable_timing;

    //! Automatically fill capture timestamps of input frames with invocation time.
    bool enable_auto_cts;

    //! Profile moving average of frames being written.
    bool enable_profiling;

    //! Profiler configuration.
    audio::ProfilerConfig profiler_config;

    SenderConfig()
        : resampler_backend(audio::ResamplerBackend_Default)
        , resampler_profile(audio::ResamplerProfile_Medium)
        , input_sample_spec(DefaultSampleSpec)
        , packet_length(DefaultPacketLength)
        , payload_type(rtp::PayloadType_L16_Stereo)
        , enable_interleaving(false)
        , enable_timing(false)
        , enable_auto_cts(false)
        , enable_profiling(false) {
    }
};

//! Receiver session parameters.
//! @remarks
//!  Defines per-session receiver parameters.
struct ReceiverSessionConfig {
    //! Target latency, nanoseconds.
    core::nanoseconds_t target_latency;

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
        , payload_type(0)
        , resampler_backend(audio::ResamplerBackend_Default)
        , resampler_profile(audio::ResamplerProfile_Medium) {
        latency_monitor.deduce_latency_tolerance(DefaultLatency);
    }

    //! Automatically deduce resampler backend from FreqEstimator config.
    void deduce_resampler_backend() {
        if (latency_monitor.fe_enable
            && latency_monitor.fe_profile == audio::FreqEstimatorProfile_Responsive) {
            resampler_backend = audio::ResamplerBackend_Builtin;
        } else {
            resampler_backend = audio::ResamplerBackend_Default;
        }
    }
};

//! Receiver common parameters.
//! @remarks
//!  Defines receiver parameters common for all sessions.
struct ReceiverCommonConfig {
    //! Output sample spec
    audio::SampleSpec output_sample_spec;

    //! Constrain receiver speed using a CPU timer according to the sample rate.
    bool enable_timing;

    //! Automatically invoke reclock before returning frames with invocation time.
    bool enable_auto_reclock;

    //! Profile moving average of frames being written.
    bool enable_profiling;

    //! Profiler configuration.
    audio::ProfilerConfig profiler_config;

    //! Insert weird beeps instead of silence on packet loss.
    bool enable_beeping;

    ReceiverCommonConfig()
        : output_sample_spec(DefaultSampleSpec)
        , enable_timing(false)
        , enable_auto_reclock(false)
        , enable_profiling(false)
        , enable_beeping(false) {
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
struct TranscoderConfig {
    //! To specify which resampling backend will be used.
    audio::ResamplerBackend resampler_backend;

    //! Resampler profile.
    audio::ResamplerProfile resampler_profile;

    //! Input sample spec
    audio::SampleSpec input_sample_spec;

    //! Output sample spec
    audio::SampleSpec output_sample_spec;

    //! Profile moving average of frames being written.
    bool enable_profiling;

    //! Profiler configuration.
    audio::ProfilerConfig profiler_config;

    TranscoderConfig()
        : resampler_backend(audio::ResamplerBackend_Default)
        , resampler_profile(audio::ResamplerProfile_Medium)
        , input_sample_spec(DefaultSampleSpec)
        , output_sample_spec(DefaultSampleSpec)
        , enable_profiling(false) {
    }
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CONFIG_H_
