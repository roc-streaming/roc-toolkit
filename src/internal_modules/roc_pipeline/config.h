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
#include "roc_audio/feedback_monitor.h"
#include "roc_audio/jitter_meter.h"
#include "roc_audio/latency_config.h"
#include "roc_audio/plc_config.h"
#include "roc_audio/profiler.h"
#include "roc_audio/resampler_config.h"
#include "roc_audio/sample_spec.h"
#include "roc_audio/watchdog.h"
#include "roc_core/attributes.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_dbgio/csv_dumper.h"
#include "roc_fec/block_reader.h"
#include "roc_fec/block_writer.h"
#include "roc_fec/codec_config.h"
#include "roc_packet/units.h"
#include "roc_pipeline/pipeline_loop.h"
#include "roc_rtcp/config.h"
#include "roc_rtp/filter.h"

namespace roc {
namespace pipeline {

//! Default sample specification.
static const audio::SampleSpec DefaultSampleSpec(44100,
                                                 audio::PcmSubformat_Raw,
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

//! Parameters of sender sink and sender session.
struct SenderSinkConfig {
    //! Input sample spec
    audio::SampleSpec input_sample_spec;

    //! Task processing parameters.
    PipelineLoopConfig pipeline_loop;

    //! RTP payload type for audio packets.
    unsigned payload_type;

    //! Packet length, in nanoseconds.
    core::nanoseconds_t packet_length;

    //! FEC writer parameters.
    fec::BlockWriterConfig fec_writer;

    //! FEC encoder parameters.
    fec::CodecConfig fec_encoder;

    //! Feedback parameters.
    audio::FeedbackConfig feedback;

    //! Latency parameters.
    audio::LatencyConfig latency;

    //! Freq estimator parameters.
    audio::FreqEstimatorConfig freq_est;

    //! Resampler parameters.
    audio::ResamplerConfig resampler;

    //! Profiler configuration.
    audio::ProfilerConfig profiler;

    //! RTCP config.
    rtcp::Config rtcp;

    //! Block write operations on CPU timer according to the sample rate.
    bool enable_cpu_clock;

    //! Automatically fill capture timestamps of input frames with invocation time.
    bool enable_auto_cts;

    //! Interleave packets.
    bool enable_interleaving;

    //! Profile moving average of frames being written.
    bool enable_profiling;

    //! Parameters for a logger in csv format with some run-time metrics.
    dbgio::CsvConfig dumper;

    //! Initialize config.
    SenderSinkConfig();

    //! Fill unset values with defaults.
    ROC_ATTR_NODISCARD bool deduce_defaults(audio::ProcessorMap& processor_map);
};

//! Parameters of sender slot.
struct SenderSlotConfig {
    //! Initialize config.
    SenderSlotConfig();

    //! Fill unset values with defaults.
    ROC_ATTR_NODISCARD bool deduce_defaults();
};

//! Parameters common for all receiver sessions.
struct ReceiverCommonConfig {
    //! Output sample spec.
    audio::SampleSpec output_sample_spec;

    //! Profiler configuration.
    audio::ProfilerConfig profiler;

    //! RTP filter parameters.
    rtp::FilterConfig rtp_filter;

    //! RTCP config.
    rtcp::Config rtcp;

    //! Block read operations on CPU timer according to the sample rate.
    bool enable_cpu_clock;

    //! Automatically invoke reclock before returning frames with invocation time.
    bool enable_auto_reclock;

    //! Profile moving average of frames being written.
    bool enable_profiling;

    //! Parameters for a logger in csv format with some run-time metrics.
    dbgio::CsvConfig dumper;

    //! Initialize config.
    ReceiverCommonConfig();

    //! Fill unset values with defaults.
    ROC_ATTR_NODISCARD bool deduce_defaults(audio::ProcessorMap& processor_map);
};

//! Parameters of receiver session.
struct ReceiverSessionConfig {
    //! Packet payload type.
    unsigned int payload_type;

    //! FEC reader parameters.
    fec::BlockReaderConfig fec_reader;

    //! FEC decoder parameters.
    fec::CodecConfig fec_decoder;

    //! PLC parameters.
    audio::PlcConfig plc;

    //! Latency parameters.
    audio::LatencyConfig latency;

    //! Jitter meter parameters.
    audio::JitterMeterConfig jitter_meter;

    //! Freq estimator parameters.
    audio::FreqEstimatorConfig freq_est;

    //! Resampler parameters.
    audio::ResamplerConfig resampler;

    //! Watchdog parameters.
    audio::WatchdogConfig watchdog;

    //! Initialize config.
    ReceiverSessionConfig();

    //! Fill unset values with defaults.
    ROC_ATTR_NODISCARD bool deduce_defaults(audio::ProcessorMap& processor_map);
};

//! Parameters of receiver session.
//! Top-level config, actual settings are stored in sub-configs.
struct ReceiverSourceConfig {
    //! Task processing parameters.
    PipelineLoopConfig pipeline_loop;

    //! Parameters common for all sessions.
    ReceiverCommonConfig common;

    //! Default parameters for a session.
    ReceiverSessionConfig session_defaults;

    //! Initialize config.
    ReceiverSourceConfig();

    //! Fill unset values with defaults.
    ROC_ATTR_NODISCARD bool deduce_defaults(audio::ProcessorMap& processor_map);
};

//! Parameters of receiver slot.
struct ReceiverSlotConfig {
    //! Enable routing packets to multiple sessions within slot.
    bool enable_routing;

    //! Initialize config.
    ReceiverSlotConfig();

    //! Fill unset values with defaults.
    ROC_ATTR_NODISCARD bool deduce_defaults();
};

//! Converter parameters.
struct TranscoderConfig {
    //! Input sample spec
    audio::SampleSpec input_sample_spec;

    //! Output sample spec
    audio::SampleSpec output_sample_spec;

    //! Resampler parameters.
    audio::ResamplerConfig resampler;

    //! Profiler configuration.
    audio::ProfilerConfig profiler;

    //! Profile moving average of frames being written.
    bool enable_profiling;

    //! Initialize config.
    TranscoderConfig();

    //! Fill unset values with defaults.
    ROC_ATTR_NODISCARD bool deduce_defaults(audio::ProcessorMap& processor_map);
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CONFIG_H_
