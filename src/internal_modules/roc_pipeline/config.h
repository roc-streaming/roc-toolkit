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
#include "roc_audio/latency_config.h"
#include "roc_audio/profiler.h"
#include "roc_audio/resampler_config.h"
#include "roc_audio/sample_spec.h"
#include "roc_audio/watchdog.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_fec/codec_config.h"
#include "roc_fec/reader.h"
#include "roc_fec/writer.h"
#include "roc_packet/units.h"
#include "roc_pipeline/pipeline_loop.h"
#include "roc_rtcp/config.h"
#include "roc_rtp/filter.h"

namespace roc {
namespace pipeline {

//! Default sample rate, number of samples per second.
const size_t DefaultSampleRate = 44100;

//! Default sample specification.
static const audio::SampleSpec DefaultSampleSpec(DefaultSampleRate,
                                                 audio::Sample_RawFormat,
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

//! Sender parameters.
struct SenderConfig {
    //! Task processing parameters.
    PipelineLoopConfig pipeline_loop;

    //! Latency parameters.
    audio::LatencyConfig latency;

    //! Resampler parameters.
    audio::ResamplerConfig resampler;

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
    audio::ProfilerConfig profiler;

    //! RTCP config.
    rtcp::Config rtcp;

    //! Initialize config.
    SenderConfig();

    //! Fill unset values with defaults.
    void deduce_defaults();
};

//! Receiver session parameters.
//! @remarks
//!  Defines per-session receiver parameters.
struct ReceiverSessionConfig {
    //! Packet payload type.
    unsigned int payload_type;

    //! FEC reader parameters.
    fec::ReaderConfig fec_reader;

    //! FEC decoder parameters.
    fec::CodecConfig fec_decoder;

    //! Latency parameters.
    audio::LatencyConfig latency;

    //! Watchdog parameters.
    audio::WatchdogConfig watchdog;

    //! Resampler parameters.
    audio::ResamplerConfig resampler;

    //! Initialize config.
    ReceiverSessionConfig();

    //! Fill unset values with defaults.
    void deduce_defaults();
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
    audio::ProfilerConfig profiler;

    //! RTP filter parameters.
    rtp::FilterConfig rtp_filter;

    //! RTCP config.
    rtcp::Config rtcp;

    //! Insert weird beeps instead of silence on packet loss.
    bool enable_beeping;

    //! Initialize config.
    ReceiverCommonConfig();

    //! Fill unset values with defaults.
    void deduce_defaults();
};

//! Receiver parameters.
struct ReceiverConfig {
    //! Task processing parameters.
    PipelineLoopConfig pipeline_loop;

    //! Default parameters for receiver session.
    ReceiverSessionConfig default_session;

    //! Parameters common for all sessions.
    ReceiverCommonConfig common;

    //! Initialize config.
    ReceiverConfig();

    //! Fill unset values with defaults.
    void deduce_defaults();
};

//! Converter parameters.
struct TranscoderConfig {
    //! Resampler parameters.
    audio::ResamplerConfig resampler;

    //! Input sample spec
    audio::SampleSpec input_sample_spec;

    //! Output sample spec
    audio::SampleSpec output_sample_spec;

    //! Profile moving average of frames being written.
    bool enable_profiling;

    //! Profiler configuration.
    audio::ProfilerConfig profiler;

    //! Initialize config.
    TranscoderConfig();

    //! Fill unset values with defaults.
    void deduce_defaults();
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CONFIG_H_
