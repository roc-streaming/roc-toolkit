/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "adapters.h"

#include "roc_address/interface.h"
#include "roc_audio/channel_defs.h"
#include "roc_audio/freq_estimator.h"
#include "roc_audio/resampler_backend.h"
#include "roc_audio/resampler_profile.h"
#include "roc_core/attributes.h"
#include "roc_core/log.h"

namespace roc {
namespace api {

// Note: ROC_ATTR_NO_SANITIZE_UB is used from *_from_user() functions because we don't
// want sanitizers to fail us when enums contain arbitrary values; we correctly handle
// all these cases.

#ifdef __clang__
// On clang, we use switches with original enum value. This allows compiler to warn us
// if we forget to list a value.
template <class T> ROC_ATTR_NO_SANITIZE_UB T enum_from_user(T t) {
    return t;
}
#else
// On other compilers, we use switches with enum value casted to int. This prevents
// some broken compiler versions (e.g. older gcc) to optimize out the code after switch
// which corresponds to unmatched value. Clang does not have this problem, so we don't
// use this hack on it to benefit from the warnings.
template <class T> ROC_ATTR_NO_SANITIZE_UB int enum_from_user(T t) {
    return t;
}
#endif

ROC_ATTR_NO_SANITIZE_UB
bool context_config_from_user(node::ContextConfig& out, const roc_context_config& in) {
    if (in.max_packet_size != 0) {
        out.max_packet_size = in.max_packet_size;
    }

    if (in.max_frame_size != 0) {
        out.max_frame_size = in.max_frame_size;
    }

    return true;
}

ROC_ATTR_NO_SANITIZE_UB
bool sender_config_from_user(node::Context& context,
                             pipeline::SenderConfig& out,
                             const roc_sender_config& in) {
    if (!sample_spec_from_user(out.input_sample_spec, in.frame_encoding)) {
        roc_log(LogError, "bad configuration: invalid roc_sender_config.frame_encoding");
        return false;
    }

    if (in.packet_encoding != 0) {
        if (!packet_encoding_from_user(out.payload_type, in.packet_encoding)) {
            roc_log(LogError,
                    "bad configuration: invalid roc_sender_config.packet_encoding:"
                    " should be zero or valid encoding id");
            return false;
        }
        const rtp::Format* format = context.format_map().find_by_pt(out.payload_type);
        if (!format) {
            roc_log(LogError,
                    "bad configuration: invalid roc_sender_config.packet_encoding:"
                    " no built-in or registered encoding found with id %u",
                    (unsigned)out.payload_type);
            return false;
        }
    } else {
        const rtp::Format* format =
            context.format_map().find_by_spec(out.input_sample_spec);
        if (!format) {
            roc_log(LogError,
                    "bad configuration:"
                    " failed to select packet_encoding matching frame_encoding,"
                    " set roc_sender_config.packet_encoding manually");
            return false;
        }
        out.payload_type = format->payload_type;
    }

    if (in.packet_length != 0) {
        out.packet_length = (core::nanoseconds_t)in.packet_length;
    }

    out.enable_timing = false;
    out.enable_auto_cts = true;

    out.enable_interleaving = in.packet_interleaving;

    if (!fec_encoding_from_user(out.fec_encoder.scheme, in.fec_encoding)) {
        roc_log(LogError,
                "bad configuration: invalid roc_sender_config.fec_encoding:"
                " should be valid enum value");
        return false;
    }

    if (in.fec_block_source_packets != 0 || in.fec_block_repair_packets != 0) {
        out.fec_writer.n_source_packets = in.fec_block_source_packets;
        out.fec_writer.n_repair_packets = in.fec_block_repair_packets;
    }

    if (!clock_source_from_user(out.enable_timing, in.clock_source)) {
        roc_log(LogError,
                "bad configuration: invalid roc_sender_config.clock_source:"
                " should be valid enum value");
        return false;
    }

    if (!resampler_backend_from_user(out.resampler_backend, in.resampler_backend)) {
        roc_log(LogError,
                "bad configuration: invalid roc_sender_config.resampler_backend:"
                " should be valid enum value");
        return false;
    }

    if (!resampler_profile_from_user(out.resampler_profile, in.resampler_profile)) {
        roc_log(LogError,
                "bad configuration: invalid roc_sender_config.resampler_profile:"
                " should be valid enum value");
        return false;
    }

    return true;
}

ROC_ATTR_NO_SANITIZE_UB
bool receiver_config_from_user(node::Context&,
                               pipeline::ReceiverConfig& out,
                               const roc_receiver_config& in) {
    if (in.target_latency != 0) {
        out.default_session.target_latency = (core::nanoseconds_t)in.target_latency;
    }

    if (in.latency_tolerance != 0) {
        out.default_session.latency_monitor.latency_tolerance =
            (core::nanoseconds_t)in.latency_tolerance;
    } else {
        out.default_session.latency_monitor.deduce_latency_tolerance(
            out.default_session.target_latency);
    }

    if (in.no_playback_timeout < 0) {
        out.default_session.watchdog.no_playback_timeout = 0;
    } else if (in.no_playback_timeout > 0) {
        out.default_session.watchdog.no_playback_timeout = in.no_playback_timeout;
    }

    if (in.choppy_playback_timeout < 0) {
        out.default_session.watchdog.choppy_playback_timeout = 0;
    } else if (in.choppy_playback_timeout > 0) {
        out.default_session.watchdog.choppy_playback_timeout = in.choppy_playback_timeout;

        out.default_session.watchdog.deduce_choppy_playback_window(
            out.default_session.watchdog.choppy_playback_timeout);
    }

    out.common.enable_timing = false;
    out.common.enable_auto_reclock = true;

    if (!sample_spec_from_user(out.common.output_sample_spec, in.frame_encoding)) {
        roc_log(LogError,
                "bad configuration: invalid roc_receiver_config.frame_encoding");
        return false;
    }

    if (!clock_source_from_user(out.common.enable_timing, in.clock_source)) {
        roc_log(LogError,
                "bad configuration: invalid roc_receiver_config.clock_source:"
                " should be valid enum value");
        return false;
    }

    if (!clock_sync_backend_from_user(out.default_session.latency_monitor.fe_enable,
                                      in.clock_sync_backend)) {
        roc_log(LogError,
                "bad configuration: invalid roc_receiver_config.clock_sync_backend:"
                " should be valid enum value");
        return false;
    }

    if (in.clock_sync_profile != ROC_CLOCK_SYNC_PROFILE_DEFAULT) {
        if (!clock_sync_profile_from_user(out.default_session.latency_monitor.fe_profile,
                                          in.clock_sync_profile)) {
            roc_log(LogError,
                    "bad configuration: invalid roc_receiver_config.clock_sync_profile:"
                    " should be valid enum value");
            return false;
        }
    } else {
        if (out.default_session.latency_monitor.fe_enable) {
            out.default_session.latency_monitor.deduce_fe_profile(
                out.default_session.target_latency);
        }
    }

    if (in.resampler_backend != ROC_RESAMPLER_BACKEND_DEFAULT) {
        if (!resampler_backend_from_user(out.default_session.resampler_backend,
                                         in.resampler_backend)) {
            roc_log(LogError,
                    "bad configuration: invalid roc_receiver_config.resampler_backend:"
                    " should be valid enum value");
            return false;
        }
    } else {
        out.default_session.deduce_resampler_backend();
    }

    if (!resampler_profile_from_user(out.default_session.resampler_profile,
                                     in.resampler_profile)) {
        roc_log(LogError,
                "bad configuration: invalid roc_receiver_config.resampler_profile:"
                " should be valid enum value");
        return false;
    }

    return true;
}

ROC_ATTR_NO_SANITIZE_UB bool
sender_interface_config_from_user(netio::UdpSenderConfig& out,
                                  const roc_interface_config& in) {
    if (in.outgoing_address[0] != '\0') {
        if (!out.bind_address.set_host_port_auto(in.outgoing_address, 0)) {
            roc_log(LogError,
                    "bad configuration: invalid roc_interface_config.outgoing_address:"
                    " should be either empty or valid IPv4/IPv6 address");
            return false;
        }
    }

    if (in.multicast_group[0] != '\0') {
        roc_log(LogError,
                "bad configuration: invalid roc_interface_config.multicast_group:"
                " should be empty for sender");
        return false;
    }

    out.reuseaddr = (in.reuse_address != 0);

    return true;
}

ROC_ATTR_NO_SANITIZE_UB bool
receiver_interface_config_from_user(netio::UdpReceiverConfig& out,
                                    const roc_interface_config& in) {
    if (in.outgoing_address[0] != '\0') {
        if (!out.bind_address.set_host_port_auto(in.outgoing_address, 0)) {
            roc_log(LogError,
                    "bad configuration: invalid roc_interface_config.outgoing_address:"
                    " should be either empty or valid IPv4/IPv6 address");
            return false;
        }
    }

    if (in.multicast_group[0] != '\0') {
        if (strlen(in.multicast_group) >= sizeof(out.multicast_interface)) {
            roc_log(LogError,
                    "bad configuration: invalid roc_interface_config.multicast_group:"
                    " should be no longer than %d characters",
                    (int)sizeof(out.multicast_interface) - 1);
            return false;
        }

        address::SocketAddr addr;
        if (!addr.set_host_port_auto(in.multicast_group, 0)) {
            roc_log(LogError,
                    "bad configuration: invalid roc_interface_config.multicast_group:"
                    " should be either empty or valid IPv4/IPv6 address");
            return false;
        }

        strcpy(out.multicast_interface, in.multicast_group);
    }

    out.reuseaddr = (in.reuse_address != 0);

    return true;
}

ROC_ATTR_NO_SANITIZE_UB
bool sample_spec_from_user(audio::SampleSpec& out, const roc_media_encoding& in) {
    if (in.rate != 0) {
        out.set_sample_rate(in.rate);
    } else {
        roc_log(LogError,
                "bad configuration: invalid roc_media_encoding.rate:"
                " should be non-zero");
        return false;
    }

    if (in.format != ROC_FORMAT_PCM_FLOAT32) {
        roc_log(LogError,
                "bad configuration: invalid roc_media_encoding.format:"
                " should be valid enum value");
        return false;
    }

    if (in.channels != 0) {
        if (in.channels == ROC_CHANNEL_LAYOUT_MULTITRACK) {
            if (in.tracks == 0) {
                roc_log(LogError,
                        "bad configuration: invalid roc_media_encoding:"
                        " if channels is ROC_CHANNEL_LAYOUT_MULTITRACK,"
                        " then tracks should be non-zero");
                return false;
            }
            if (in.tracks > audio::ChannelSet::max_channels()) {
                roc_log(LogError,
                        "bad configuration: invalid roc_media_encoding:"
                        " invalid tracks count: got=%u expected=[1;%u]",
                        (unsigned)in.tracks, (unsigned)audio::ChannelSet::max_channels());
                return false;
            }
        } else {
            if (in.tracks != 0) {
                roc_log(LogError,
                        "bad configuration: invalid roc_media_encoding:"
                        " if channels is not ROC_CHANNEL_LAYOUT_MULTITRACK,"
                        " then tracks should be zero");
                return false;
            }
        }
        if (!channel_set_from_user(out.channel_set(), in.channels, in.tracks)) {
            roc_log(LogError,
                    "bad configuration: invalid roc_media_encoding.channels:"
                    " should be valid enum value");
            return false;
        }
    } else {
        roc_log(LogError,
                "bad configuration: invalid roc_media_encoding.channels:"
                " should be non-zero");
        return false;
    }

    return true;
}

ROC_ATTR_NO_SANITIZE_UB
bool channel_set_from_user(audio::ChannelSet& out,
                           roc_channel_layout in,
                           unsigned int in_tracks) {
    switch (enum_from_user(in)) {
    case ROC_CHANNEL_LAYOUT_MULTITRACK:
        out.clear();
        out.set_layout(audio::ChanLayout_Multitrack);
        out.set_order(audio::ChanOrder_None);
        out.set_channel_range(0, in_tracks - 1, true);
        return true;

    case ROC_CHANNEL_LAYOUT_MONO:
        out.clear();
        out.set_layout(audio::ChanLayout_Surround);
        out.set_order(audio::ChanOrder_Smpte);
        out.set_channel_mask(audio::ChanMask_Surround_Mono);
        return true;

    case ROC_CHANNEL_LAYOUT_STEREO:
        out.clear();
        out.set_layout(audio::ChanLayout_Surround);
        out.set_order(audio::ChanOrder_Smpte);
        out.set_channel_mask(audio::ChanMask_Surround_Stereo);
        return true;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool clock_source_from_user(bool& out_timing, roc_clock_source in) {
    switch (enum_from_user(in)) {
    case ROC_CLOCK_SOURCE_EXTERNAL:
        out_timing = false;
        return true;

    case ROC_CLOCK_SOURCE_INTERNAL:
        out_timing = true;
        return true;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool clock_sync_backend_from_user(bool& out_fe, roc_clock_sync_backend in) {
    switch (enum_from_user(in)) {
    case ROC_CLOCK_SYNC_BACKEND_DISABLE:
        out_fe = false;
        return true;

    case ROC_CLOCK_SYNC_BACKEND_DEFAULT:
    case ROC_CLOCK_SYNC_BACKEND_NIQ:
        out_fe = true;
        return true;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool clock_sync_profile_from_user(audio::FreqEstimatorProfile& out,
                                  roc_clock_sync_profile in) {
    switch (enum_from_user(in)) {
    case ROC_CLOCK_SYNC_PROFILE_DEFAULT:
    case ROC_CLOCK_SYNC_PROFILE_RESPONSIVE:
        out = audio::FreqEstimatorProfile_Responsive;
        return true;

    case ROC_CLOCK_SYNC_PROFILE_GRADUAL:
        out = audio::FreqEstimatorProfile_Gradual;
        return true;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool resampler_backend_from_user(audio::ResamplerBackend& out, roc_resampler_backend in) {
    switch (enum_from_user(in)) {
    case ROC_RESAMPLER_BACKEND_DEFAULT:
        out = audio::ResamplerBackend_Default;
        return true;

    case ROC_RESAMPLER_BACKEND_BUILTIN:
        out = audio::ResamplerBackend_Builtin;
        return true;

    case ROC_RESAMPLER_BACKEND_SPEEX:
        out = audio::ResamplerBackend_Speex;
        return true;

    case ROC_RESAMPLER_BACKEND_SPEEXDEC:
        out = audio::ResamplerBackend_SpeexDec;
        return true;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool resampler_profile_from_user(audio::ResamplerProfile& out, roc_resampler_profile in) {
    switch (enum_from_user(in)) {
    case ROC_RESAMPLER_PROFILE_LOW:
        out = audio::ResamplerProfile_Low;
        return true;

    case ROC_RESAMPLER_PROFILE_DEFAULT:
    case ROC_RESAMPLER_PROFILE_MEDIUM:
        out = audio::ResamplerProfile_Medium;
        return true;

    case ROC_RESAMPLER_PROFILE_HIGH:
        out = audio::ResamplerProfile_High;
        return true;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool packet_encoding_from_user(unsigned& out_pt, roc_packet_encoding in) {
    switch (enum_from_user(in)) {
    case ROC_PACKET_ENCODING_AVP_L16_MONO:
        out_pt = rtp::PayloadType_L16_Mono;
        return true;

    case ROC_PACKET_ENCODING_AVP_L16_STEREO:
        out_pt = rtp::PayloadType_L16_Stereo;
        return true;
    }

    out_pt = in;
    return true;
}

ROC_ATTR_NO_SANITIZE_UB
bool fec_encoding_from_user(packet::FecScheme& out, roc_fec_encoding in) {
    switch (enum_from_user(in)) {
    case ROC_FEC_ENCODING_DISABLE:
        out = packet::FEC_None;
        return true;

    case ROC_FEC_ENCODING_DEFAULT:
    case ROC_FEC_ENCODING_RS8M:
        out = packet::FEC_ReedSolomon_M8;
        return true;

    case ROC_FEC_ENCODING_LDPC_STAIRCASE:
        out = packet::FEC_LDPC_Staircase;
        return true;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool interface_from_user(address::Interface& out, const roc_interface& in) {
    switch (enum_from_user(in)) {
    case ROC_INTERFACE_CONSOLIDATED:
        out = address::Iface_Consolidated;
        return true;

    case ROC_INTERFACE_AUDIO_SOURCE:
        out = address::Iface_AudioSource;
        return true;

    case ROC_INTERFACE_AUDIO_REPAIR:
        out = address::Iface_AudioRepair;
        return true;

    case ROC_INTERFACE_AUDIO_CONTROL:
        out = address::Iface_AudioControl;
        return true;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool proto_from_user(address::Protocol& out, const roc_protocol& in) {
    switch (enum_from_user(in)) {
    case ROC_PROTO_RTSP:
        out = address::Proto_RTSP;
        return true;

    case ROC_PROTO_RTP:
        out = address::Proto_RTP;
        return true;

    case ROC_PROTO_RTP_RS8M_SOURCE:
        out = address::Proto_RTP_RS8M_Source;
        return true;

    case ROC_PROTO_RS8M_REPAIR:
        out = address::Proto_RS8M_Repair;
        return true;

    case ROC_PROTO_RTP_LDPC_SOURCE:
        out = address::Proto_RTP_LDPC_Source;
        return true;

    case ROC_PROTO_LDPC_REPAIR:
        out = address::Proto_LDPC_Repair;
        return true;

    case ROC_PROTO_RTCP:
        out = address::Proto_RTCP;
        return true;
    }

    return false;
}

bool proto_to_user(roc_protocol& out, address::Protocol in) {
    switch (enum_from_user(in)) {
    case address::Proto_RTSP:
        out = ROC_PROTO_RTSP;
        return true;

    case address::Proto_RTP:
        out = ROC_PROTO_RTP;
        return true;

    case address::Proto_RTP_RS8M_Source:
        out = ROC_PROTO_RTP_RS8M_SOURCE;
        return true;

    case address::Proto_RS8M_Repair:
        out = ROC_PROTO_RS8M_REPAIR;
        return true;

    case address::Proto_RTP_LDPC_Source:
        out = ROC_PROTO_RTP_LDPC_SOURCE;
        return true;

    case address::Proto_LDPC_Repair:
        out = ROC_PROTO_LDPC_REPAIR;
        return true;

    case address::Proto_RTCP:
        out = ROC_PROTO_RTCP;
        return true;

    case address::Proto_None:
        break;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
void receiver_slot_metrics_to_user(roc_receiver_metrics& out,
                                   const pipeline::ReceiverSlotMetrics& in) {
    out.num_sessions = (unsigned)in.num_sessions;
}

ROC_ATTR_NO_SANITIZE_UB
void receiver_session_metrics_to_user(
    const pipeline::ReceiverSessionMetrics& sess_metrics,
    size_t sess_index,
    void* sess_arg) {
    roc_session_metrics& out = *((roc_session_metrics*)sess_arg + sess_index);

    memset(&out, 0, sizeof(out));

    if (sess_metrics.latency.niq_latency > 0) {
        out.niq_latency = (unsigned long long)sess_metrics.latency.niq_latency;
    }

    if (sess_metrics.latency.e2e_latency > 0) {
        out.e2e_latency = (unsigned long long)sess_metrics.latency.e2e_latency;
    }
}

ROC_ATTR_NO_SANITIZE_UB
void sender_metrics_to_user(roc_sender_metrics& out,
                            const pipeline::SenderSlotMetrics& in_slot,
                            const pipeline::SenderSessionMetrics& in_sess) {
    memset(&out, 0, sizeof(out));

    (void)in_slot;
    (void)in_sess;
}

ROC_ATTR_NO_SANITIZE_UB
LogLevel log_level_from_user(roc_log_level in) {
    switch (enum_from_user(in)) {
    case ROC_LOG_NONE:
        return LogNone;

    case ROC_LOG_ERROR:
        return LogError;

    case ROC_LOG_INFO:
        return LogInfo;

    case ROC_LOG_DEBUG:
        return LogDebug;

    case ROC_LOG_TRACE:
        return LogTrace;
    }

    return LogError;
}

roc_log_level log_level_to_user(LogLevel in) {
    switch (in) {
    case LogNone:
        return ROC_LOG_NONE;

    case LogError:
        return ROC_LOG_ERROR;

    case LogInfo:
        return ROC_LOG_INFO;

    case LogDebug:
        return ROC_LOG_DEBUG;

    case LogTrace:
        return ROC_LOG_TRACE;
    }

    return ROC_LOG_ERROR;
}

void log_message_to_user(roc_log_message& out, const core::LogMessage& in) {
    out.level = log_level_to_user(in.level);
    out.module = in.module;
    out.file = in.file;
    out.line = in.line;
    out.time = (unsigned long long)in.time;
    out.pid = (unsigned long long)in.pid;
    out.tid = (unsigned long long)in.tid;
    out.text = in.text;
}

} // namespace api
} // namespace roc
