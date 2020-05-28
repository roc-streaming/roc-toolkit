/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "config_helpers.h"

#include "roc_audio/resampler_profile.h"
#include "roc_core/log.h"

namespace roc {
namespace api {

bool context_config_from_user(peer::ContextConfig& out, const roc_context_config& in) {
    if (in.max_packet_size != 0) {
        out.max_packet_size = in.max_packet_size;
    }

    if (in.max_frame_size / sizeof(audio::sample_t) != 0) {
        out.max_frame_size = in.max_frame_size;
    }

    return true;
}

bool sender_config_from_user(pipeline::SenderConfig& out, const roc_sender_config& in) {
    if (in.frame_sample_rate != 0) {
        out.input_sample_rate = in.frame_sample_rate;
    } else {
        roc_log(LogError, "bad configuration: invalid frame_sample_rate");
        return false;
    }

    if (in.frame_channels != ROC_CHANNEL_SET_STEREO) {
        roc_log(LogError, "bad configuration: invalid frame_channels");
        return false;
    }

    if (in.frame_encoding != ROC_FRAME_ENCODING_PCM_FLOAT) {
        roc_log(LogError, "bad configuration: invalid frame_encoding");
        return false;
    }

    if (in.packet_sample_rate != 0 && in.packet_sample_rate != 44100) {
        roc_log(LogError,
                "bad configuration:"
                " invalid packet_sample_rate, only 44100 is currently supported");
        return false;
    }

    if (in.packet_channels != 0 && in.packet_channels != ROC_CHANNEL_SET_STEREO) {
        roc_log(LogError, "bad configuration: invalid packet_channels");
        return false;
    }

    if (in.packet_encoding != 0 && in.packet_encoding != ROC_PACKET_ENCODING_AVP_L16) {
        roc_log(LogError, "bad configuration: invalid packet_encoding");
        return false;
    }

    if (in.packet_length != 0) {
        out.packet_length = (core::nanoseconds_t)in.packet_length;
    }

    out.interleaving = in.packet_interleaving;
    out.timing = (in.clock_source == ROC_CLOCK_INTERNAL);

    out.resampling = (in.resampler_profile != ROC_RESAMPLER_PROFILE_DISABLE);

    switch ((int)in.resampler_backend) {
    case ROC_RESAMPLER_BACKEND_DEFAULT:
        out.resampler_backend = audio::ResamplerBackend_Default;
        break;
    case ROC_RESAMPLER_BACKEND_BUILTIN:
        out.resampler_backend = audio::ResamplerBackend_Builtin;
        break;
    case ROC_RESAMPLER_BACKEND_SPEEX:
        out.resampler_backend = audio::ResamplerBackend_Speex;
        break;
    default:
        roc_log(LogError, "bad configuration: invalid resampler_backend");
        return false;
    }

    switch ((int)in.resampler_profile) {
    case ROC_RESAMPLER_PROFILE_DISABLE:
        break;
    case ROC_RESAMPLER_PROFILE_LOW:
        out.resampler_profile = audio::ResamplerProfile_Low;
        break;
    case ROC_RESAMPLER_PROFILE_DEFAULT:
    case ROC_RESAMPLER_PROFILE_MEDIUM:
        out.resampler_profile = audio::ResamplerProfile_Medium;
        break;
    case ROC_RESAMPLER_PROFILE_HIGH:
        out.resampler_profile = audio::ResamplerProfile_High;
        break;
    default:
        roc_log(LogError, "bad configuration: invalid resampler_profile");
        return false;
    }

    switch ((int)in.fec_code) {
    case ROC_FEC_DISABLE:
        out.fec_encoder.scheme = packet::FEC_None;
        break;
    case ROC_FEC_DEFAULT:
    case ROC_FEC_RS8M:
        out.fec_encoder.scheme = packet::FEC_ReedSolomon_M8;
        break;
    case ROC_FEC_LDPC_STAIRCASE:
        out.fec_encoder.scheme = packet::FEC_LDPC_Staircase;
        break;
    default:
        roc_log(LogError, "bad configuration: invalid fec_scheme");
        return false;
    }

    if (in.fec_block_source_packets != 0 || in.fec_block_repair_packets != 0) {
        out.fec_writer.n_source_packets = in.fec_block_source_packets;
        out.fec_writer.n_repair_packets = in.fec_block_repair_packets;
    }

    return true;
}

bool receiver_config_from_user(pipeline::ReceiverConfig& out,
                               const roc_receiver_config& in) {
    if (in.frame_sample_rate != 0) {
        out.common.output_sample_rate = in.frame_sample_rate;
    } else {
        roc_log(LogError, "bad configuration: invalid frame_sample_rate");
        return false;
    }

    if (in.frame_channels != ROC_CHANNEL_SET_STEREO) {
        roc_log(LogError, "bad configuration: invalid frame_channels");
        return false;
    }

    if (in.frame_encoding != ROC_FRAME_ENCODING_PCM_FLOAT) {
        roc_log(LogError, "bad configuration: invalid frame_encoding");
        return false;
    }

    out.common.timing = (in.clock_source == ROC_CLOCK_INTERNAL);
    out.common.resampling = (in.resampler_profile != ROC_RESAMPLER_PROFILE_DISABLE);

    switch ((int)in.resampler_backend) {
    case ROC_RESAMPLER_BACKEND_DEFAULT:
        out.default_session.resampler_backend = audio::ResamplerBackend_Default;
        break;
    case ROC_RESAMPLER_BACKEND_BUILTIN:
        out.default_session.resampler_backend = audio::ResamplerBackend_Builtin;
        break;
    case ROC_RESAMPLER_BACKEND_SPEEX:
        out.default_session.resampler_backend = audio::ResamplerBackend_Speex;
        break;
    default:
        roc_log(LogError, "bad configuration: invalid resampler_backend");
        return false;
    }

    switch ((int)in.resampler_profile) {
    case ROC_RESAMPLER_PROFILE_DISABLE:
        break;
    case ROC_RESAMPLER_PROFILE_LOW:
        out.default_session.resampler_profile = audio::ResamplerProfile_Low;
        break;
    case ROC_RESAMPLER_PROFILE_DEFAULT:
    case ROC_RESAMPLER_PROFILE_MEDIUM:
        out.default_session.resampler_profile = audio::ResamplerProfile_Medium;
        break;
    case ROC_RESAMPLER_PROFILE_HIGH:
        out.default_session.resampler_profile = audio::ResamplerProfile_High;
        break;
    default:
        roc_log(LogError, "bad configuration: invalid resampler_profile");
        return false;
    }

    if (in.target_latency != 0) {
        out.default_session.target_latency = (core::nanoseconds_t)in.target_latency;

        out.default_session.latency_monitor.min_latency =
            (core::nanoseconds_t)in.target_latency * pipeline::DefaultMinLatencyFactor;

        out.default_session.latency_monitor.max_latency =
            (core::nanoseconds_t)in.target_latency * pipeline::DefaultMaxLatencyFactor;

        if (out.default_session.watchdog.no_playback_timeout
            < out.default_session.latency_monitor.max_latency) {
            out.default_session.watchdog.no_playback_timeout =
                out.default_session.latency_monitor.max_latency;
        }

        if (out.default_session.watchdog.broken_playback_timeout
            < out.default_session.latency_monitor.max_latency) {
            out.default_session.watchdog.broken_playback_timeout =
                out.default_session.latency_monitor.max_latency;
        }
    }

    if (in.max_latency_overrun != 0) {
        out.default_session.latency_monitor.min_latency =
            out.default_session.target_latency
            + (core::nanoseconds_t)in.max_latency_overrun;
    }

    if (in.max_latency_underrun != 0) {
        out.default_session.latency_monitor.max_latency =
            out.default_session.target_latency
            - (core::nanoseconds_t)in.max_latency_underrun;
    }

    if (in.no_playback_timeout < 0) {
        out.default_session.watchdog.no_playback_timeout = 0;
    } else if (in.no_playback_timeout > 0) {
        out.default_session.watchdog.no_playback_timeout = in.no_playback_timeout;
    }

    if (in.broken_playback_timeout < 0) {
        out.default_session.watchdog.broken_playback_timeout = 0;
    } else if (in.no_playback_timeout > 0) {
        out.default_session.watchdog.broken_playback_timeout = in.broken_playback_timeout;
    }

    if (in.breakage_detection_window != 0) {
        out.default_session.watchdog.breakage_detection_window =
            (core::nanoseconds_t)in.breakage_detection_window;
    }

    return true;
}

bool interface_from_user(address::Interface& out, roc_interface in) {
    switch ((int)in) {
    case ROC_INTERFACE_AUDIO_SOURCE:
        out = address::Iface_AudioSource;
        return true;

    case ROC_INTERFACE_AUDIO_REPAIR:
        out = address::Iface_AudioRepair;
        return true;

    default:
        break;
    }

    roc_log(LogError, "bad configuration: invalid interface");
    return false;
}

bool proto_from_user(address::Protocol& out, roc_protocol in) {
    switch ((int)in) {
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

    default:
        break;
    }

    roc_log(LogError, "bad configuration: invalid protocol");
    return false;
}

bool proto_to_user(roc_protocol& out, address::Protocol in) {
    switch ((int)in) {
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

    default:
        break;
    }

    roc_log(LogError, "bad configuration: invalid protocol");
    return false;
}

} // namespace api
} // namespace roc
