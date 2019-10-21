/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "private.h"

#include "roc_audio/resampler_profile.h"
#include "roc_core/log.h"
#include "roc_core/stddefs.h"

using namespace roc;

bool make_context_config(roc_context_config& out, const roc_context_config& in) {
    if (in.max_packet_size != 0) {
        out.max_packet_size = in.max_packet_size;
    } else {
        out.max_packet_size = 2048;
    }

    if (in.max_frame_size / sizeof(audio::sample_t) != 0) {
        out.max_frame_size = in.max_frame_size;
    } else {
        out.max_frame_size = 4096;
    }

    return true;
}

bool make_sender_config(pipeline::SenderConfig& out, const roc_sender_config& in) {
    if (in.frame_sample_rate != 0) {
        out.input_sample_rate = in.frame_sample_rate;
    } else {
        roc_log(LogError, "roc_config: invalid frame_sample_rate");
        return false;
    }

    if (in.frame_channels != ROC_CHANNEL_SET_STEREO) {
        roc_log(LogError, "roc_config: invalid frame_channels");
        return false;
    }

    if (in.frame_encoding != ROC_FRAME_ENCODING_PCM_FLOAT) {
        roc_log(LogError, "roc_config: invalid frame_encoding");
        return false;
    }

    if (in.packet_sample_rate != 0 && in.packet_sample_rate != 44100) {
        roc_log(
            LogError,
            "roc_config: invalid packet_sample_rate, only 44100 is currently supported");
        return false;
    }

    if (in.packet_channels != 0 && in.packet_channels != ROC_CHANNEL_SET_STEREO) {
        roc_log(LogError, "roc_config: invalid packet_channels");
        return false;
    }

    if (in.packet_encoding != 0 && in.packet_encoding != ROC_PACKET_ENCODING_AVP_L16) {
        roc_log(LogError, "roc_config: invalid packet_encoding");
        return false;
    }

    if (in.packet_length != 0) {
        out.packet_length = (core::nanoseconds_t)in.packet_length;
    }

    out.interleaving = in.packet_interleaving;
    out.timing = in.automatic_timing;

    out.resampling = (in.resampler_profile != ROC_RESAMPLER_DISABLE);

    switch ((int)in.resampler_profile) {
    case ROC_RESAMPLER_DISABLE:
        break;
    case ROC_RESAMPLER_LOW:
        out.resampler = audio::resampler_profile(audio::ResamplerProfile_Low);
        break;
    case ROC_RESAMPLER_DEFAULT:
    case ROC_RESAMPLER_MEDIUM:
        out.resampler = audio::resampler_profile(audio::ResamplerProfile_Medium);
        break;
    case ROC_RESAMPLER_HIGH:
        out.resampler = audio::resampler_profile(audio::ResamplerProfile_High);
        break;
    default:
        roc_log(LogError, "roc_config: invalid resampler_profile");
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
        roc_log(LogError, "roc_config: invalid fec_scheme");
        return false;
    }

    if (in.fec_block_source_packets != 0 || in.fec_block_repair_packets != 0) {
        out.fec_writer.n_source_packets = in.fec_block_source_packets;
        out.fec_writer.n_repair_packets = in.fec_block_repair_packets;
    }

    return true;
}

bool make_receiver_config(pipeline::ReceiverConfig& out, const roc_receiver_config& in) {
    if (in.frame_sample_rate != 0) {
        out.common.output_sample_rate = in.frame_sample_rate;
    } else {
        roc_log(LogError, "roc_config: invalid frame_sample_rate");
        return false;
    }

    if (in.frame_channels != ROC_CHANNEL_SET_STEREO) {
        roc_log(LogError, "roc_config: invalid frame_channels");
        return false;
    }

    if (in.frame_encoding != ROC_FRAME_ENCODING_PCM_FLOAT) {
        roc_log(LogError, "roc_config: invalid frame_encoding");
        return false;
    }

    out.common.timing = in.automatic_timing;

    out.common.resampling = (in.resampler_profile != ROC_RESAMPLER_DISABLE);

    switch ((int)in.resampler_profile) {
    case ROC_RESAMPLER_DISABLE:
        break;
    case ROC_RESAMPLER_LOW:
        out.default_session.resampler =
            audio::resampler_profile(audio::ResamplerProfile_Low);
        break;
    case ROC_RESAMPLER_DEFAULT:
    case ROC_RESAMPLER_MEDIUM:
        out.default_session.resampler =
            audio::resampler_profile(audio::ResamplerProfile_Medium);
        break;
    case ROC_RESAMPLER_HIGH:
        out.default_session.resampler =
            audio::resampler_profile(audio::ResamplerProfile_High);
        break;
    default:
        roc_log(LogError, "roc_config: invalid resampler_profile");
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

bool make_port_config(pipeline::PortConfig& out,
                      roc_port_type type,
                      roc_protocol proto,
                      const address::SocketAddr& addr) {
    switch ((int)type) {
    case ROC_PORT_AUDIO_SOURCE:
        switch ((int)proto) {
        case ROC_PROTO_RTP:
            out.protocol = pipeline::Proto_RTP;
            break;
        case ROC_PROTO_RTP_RS8M_SOURCE:
            out.protocol = pipeline::Proto_RTP_RSm8_Source;
            break;
        case ROC_PROTO_RTP_LDPC_SOURCE:
            out.protocol = pipeline::Proto_RTP_LDPC_Source;
            break;
        default:
            roc_log(LogError, "roc_config: invalid protocol for audio source port");
            return false;
        }
        break;

    case ROC_PORT_AUDIO_REPAIR:
        switch ((int)proto) {
        case ROC_PROTO_RS8M_REPAIR:
            out.protocol = pipeline::Proto_RSm8_Repair;
            break;
        case ROC_PROTO_LDPC_REPAIR:
            out.protocol = pipeline::Proto_LDPC_Repair;
            break;
        default:
            roc_log(LogError, "roc_config: invalid protocol for audio repair port");
            return false;
        }
        break;

    default:
        roc_log(LogError, "roc_config: invalid port type");
        return false;
    }

    out.address = addr;
    return true;
}
